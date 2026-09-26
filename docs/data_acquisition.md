# Adquisición: revisión y estado

Se inventariaron 985 archivos (sin `.git` ni `build`) y se revisaron el código de aplicación en `Core`, configuración CubeMX, arranque, linker, integración HAL/FreeRTOS, proyectos y herramientas de compilación. `Drivers` contiene las bibliotecas ST/CMSIS; `Middlewares` contiene FreeRTOS. No se hizo una auditoría línea por línea de las bibliotecas de terceros.

## Implementado

- Un único flujo de adquisición en `Core/Src/data_acquisition.c`, invocado por `StartDataAcquisitionTask`.
- Configuración central en `bms_config.h`: 8 monitores, 14 celdas/monitor y período nominal de 5 ms. El puente BQ79600 tiene dirección 0; los monitores, 1..8.
- SPI1 en PA5/PA6/PA7, CS en PA4, READY en PB2. Con STM32F303xE, HSI/PREDIV × 9 produce PCLK2 de 72 MHz; divisor SPI de 16 produce 4.5 MHz. TIM4 proporciona retardos de microsegundos.
- Wake, autoaddress incluyendo el puente, comprobación de direcciones y arranque del ADC continuo. ACTIVE_CELL = 8 corresponde a 14S. Configuración del ADC una vez por inicialización.
- Recepción de 272 bytes en bloques 128/128/16, dummy TX con 0xFF, tiempos de espera finitos, CRC y validación de longitud, registro y direcciones únicas.
- Mapeo por dirección del monitor y orden Cell1..Cell14. Conversión con signo a mV; rechazo de 0x8000 (resultado sin conversión).
- Publicación de las 112 celdas, suma, mínimo, máximo, promedio y timestamp bajo `telemetryMutex`, creado después de inicializar el kernel.
- `voltage_valid` indica éxito del último intento. En errores se conservan valores/timestamp anteriores y se incrementa `acquisition_errors`; el consumidor debe verificar validez y antigüedad. Tres errores consecutivos provocan reinicialización. Las demoras por fallos no generan ráfagas para recuperar períodos perdidos.
- Lectura de READY_PWR_SENSE y CHARGE_PWR_SENSE como niveles altos en una publicación válida; confirmar polaridad con el esquema.
- Se conserva la propagación de errores de `SpiWrite` añadida previamente por el usuario.

El driver tiene un único propietario (la tarea de adquisición). El mutex protege telemetría, no SPI; no llamar balanceo u otras funciones del driver desde tareas concurrentes sin serializar el bus.

## Pendiente de datos de hardware

La ruta de corriente PA1 → OPAMP1 → ADC1_IN3 y el promedio móvil de cuatro muestras están implementados; ver `current_sensor.md`. Falta confirmar canal del DHAB S/118, divisor y conexiones externas para habilitar amperios válidos. La lectura de TSREF/GPIO1..8 está integrada; ver `temperature_acquisition.md`. Faltan parámetros y topología de termistores para habilitar °C válidos. Confirmar también si hay 8 o 14 BQ físicos.

## Verificación

`make -j4` genera ELF, HEX y BIN sin advertencias del compilador. Pruebas de host del driver real con HAL simulado:

```sh
cc -std=c11 -Wall -Wextra -Wno-unused-parameter -fsanitize=address,undefined \
  -Itests/stubs -ICore/Inc tests/test_bq79600.c Core/Src/bq79600_driver.c \
  -o /tmp/bms-test
/tmp/bms-test
```

Cubren un vector CRC conocido, conversión con signo, orden de todas las celdas, bloques de recepción, CRC corrupto, dirección inválida/duplicada, registro incorrecto, valor 0x8000, errores TX/RX, timeout atravesando wrap de HAL_GetTick y conservación de telemetría ante fallos. No sustituyen pruebas eléctricas ni de planificación RTOS.

No se programó la placa. En banco: confirmar reloj SPI, READY/CS/MOSI, direcciones 1..8, medir cada celda contra instrumento, comprobar período de actualización y desconexión/reconexión de la cadena. El objetivo de 100 Hz requiere medición en hardware.

## Hallazgos fuera del flujo de voltaje

- CAN se inicializa pero no hay transmisión de telemetría implementada.
- SPI2 y USART3 tienen referencias/pines en CubeMX, pero no están inicializados para SD o logging. ADC1/OPAMP1 se inicializan manualmente en `current_sensor.c`.
- `simpleBalancing` es una rutina experimental separada; no se invoca ni se validó aquí. `printf` no tiene backend `__io_putchar` implementado.
- `EWARM/BMS_Master.ewp` está desactualizado respecto a FreeRTOS, CAN y fuentes de aplicación; la compilación verificada es Make/GCC.
- CubeMX no tiene SPI1 habilitado como periférico completo. Antes de regenerar, sincronizar `.ioc`, HAL SPI y lista de fuentes del Makefile; la regeneración puede sobrescribir estos ajustes.

## Referencias

Se consultó el PDF local `bq79616-q1.pdf` (SLUSE81F, junio de 2026): ACTIVE_CELL, conversión ADC, formato de respuesta y tiempo de estabilización AFE. Se trataron sus contenidos como referencia técnica.

- https://www.ti.com/lit/ds/symlink/bq79616-q1.pdf
- https://www.ti.com/lit/ds/symlink/bq79600-q1.pdf — tablas 7-5 y 7-9, interfaz SPI/FIFO y autoaddress.
