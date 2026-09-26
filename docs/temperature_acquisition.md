# Temperaturas por GPIO del BQ79616

La tarea lee voltajes y después TSREF + GPIO1..8 de cada monitor. Se conserva el período de 5 ms elegido por el usuario; el tráfico adicional puede aumentar el tiempo de iteración y requiere profiling en placa.

## Configuración pendiente

El código conserva `TOTALBOARDS=8`, porque previamente eran 8 BQ de 14 celdas. El usuario mencionó ahora 14 BQ: confirmar si son 14 monitores físicos (112 temperaturas) u 8 monitores (64 temperaturas). Los arreglos y transferencias se dimensionan con `TOTALBOARDS`; ambas variantes se probaron con HAL simulado.

Antes de usar °C deben confirmarse los termistores, pull-ups y circuito. La conversión implementada requiere NTC a GND y pull-up externo a TSREF, sin resistencia adicional en paralelo con el NTC. En `bms_config.h`:

- `BMS_NTC_R0_OHM`: resistencia nominal del NTC.
- `BMS_NTC_T0_C`: temperatura de esa resistencia nominal (25 °C provisional).
- `BMS_NTC_BETA_K`: beta del termistor.
- `BMS_NTC_PULLUP_OHM`: resistencia externa a TSREF.

R0, beta y pull-up quedan en cero hasta tener los valores reales. Esto deshabilita la conversión a °C, pero mantiene los códigos ADC disponibles y marca las temperaturas inválidas. Se presupone el mismo circuito en los ocho GPIO de todos los BQ; para sensores diferentes hay que parametrizar por canal. No se ha confirmado que todos estén cableados.

## Flujo

En `BQ79616_StartADC`:

1. Configurar ACTIVE_CELL.
2. Escribir 0x12 en GPIO_CONF1..4 (0x000E..0x0011): ocho entradas ADC, sin pulls débiles; desactiva funciones alternativas SPI/FAULT de esos GPIO.
3. Activar CONTROL2.TSREF_EN (0x030A, bit 0), solo en monitores del stack.
4. Esperar 20 ms para TSREF/AFE antes de MAIN_GO. Revisar esta espera con la capacitancia real de TSREF y filtros externos.
5. Arrancar ADC continuo y esperar 3 ms para completar las ocho posiciones GPIO.

TI indica ocho ciclos nominales de 192 µs para renovar GPIO1..8 (~1.536 ms). Las ocho temperaturas no son simultáneas; tampoco se congela la totalidad del stack al leerla.

`stackTemperatureRead` lee 18 bytes consecutivos por BQ a partir de TSREF_HI (0x058C), con seis bytes adicionales de encabezado/CRC por respuesta. Son 192 bytes para 8 BQ o 336 para 14 BQ, recibidos en bloques de hasta 128. Se reutiliza la validación de CRC, dirección y registro. El orden se determina por la dirección de cada respuesta y luego GPIO1..8.

Conversión:

```
Vgpio = raw_gpio × 152.59 µV
Vref  = raw_tsref × 169.54 µV
Rntc  = Rpullup × Vgpio / (Vref - Vgpio)
T°C   = 1 / (1/(T0°C + 273.15) + ln(Rntc/R0)/Beta) - 273.15
```

No se divide directamente el código GPIO por el código TSREF: sus escalas son distintas.

## Telemetría y errores

- `gpio_temperature_C[board][gpio]`: Celsius; índices cero equivalen a dirección BQ1 y GPIO1.
- `gpio_temperature_valid[board][gpio]`: validez individual.
- `gpio_adc_raw[board][gpio]`, `tsref_adc_raw[board]`: códigos para diagnóstico.
- `temperature_valid`: verdadero solo si todos los GPIO de todos los monitores son válidos.
- `temperature_scan_tick_ms`: última recepción completa válida de datos crudos.
- `temperature_tick_ms`: último scan completo con todas las temperaturas válidas.
- `min_temperature_C`, `max_temperature_C`, `pack_temp_C`: extremos del último scan completamente válido; `pack_temp_C` es el máximo.
- `temperature_errors`: scans inválidos o no realizados por error previo de comunicación.

Un sensor inválido devuelve NaN en su posición y no impide publicar los otros canales válidos. Un error de transporte conserva los datos anteriores y timestamps pero invalida todos los canales. El consumidor debe comprobar validez y antigüedad. Los resultados se publican bajo mutex; los cálculos logarítmicos se realizan fuera de él.

Se rechazan códigos sin conversión (0x8000), negativos, referencia nula y relaciones cercanas a rails (≤0.1% o ≥99.9%). Estos controles no sustituyen diagnóstico completo de cables ni límites de temperatura de las celdas. Fallos de sensor/configuración invalidan temperatura sin reinicializar repetidamente la comunicación BQ; fallos de transporte mantienen la recuperación existente.

## Verificación

Build Make/GCC y pruebas del driver real con HAL simulado/AddressSanitizer/UndefinedBehaviorSanitizer. Los valores 10k/beta3950 de las pruebas son una referencia sintética, no una selección de componentes para la placa.

```sh
for boards in 8 14; do
  cc -std=c11 -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined \
    -DTOTALBOARDS=$boards -DBMS_NTC_R0_OHM=10000.0f \
    -DBMS_NTC_BETA_K=3950.0f -DBMS_NTC_PULLUP_OHM=10000.0f \
    -Itests/stubs -ICore/Inc tests/test_bq79600.c Core/Src/bq79600_driver.c \
    -lm -o /tmp/bms-temp-test
  /tmp/bms-temp-test
done
```

Cobertura: conversiones conocidas a 0/25/60 °C, escalas GPIO/TSREF, orden de canales/dispositivos, CRC corrupto, referencia ausente, 0x8000, conservación de timestamps, validez individual y configuración incompleta. No probado eléctricamente.

Referencia: PDF BQ79616-Q1 aportado, SLUSE81F: §7.3.2.1.2.2, tabla de resoluciones ADC, TSREF_HI/LO, GPIO1..8_HI/LO, GPIO_CONF1..4 y CONTROL2. [Hoja de datos TI](https://www.ti.com/lit/ds/symlink/bq79616-q1.pdf).
