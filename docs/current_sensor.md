# Corriente: LEM DHAB S/118 y STM32F303RE

## Verificación

El `.ioc` identifica STM32F303RETx (LQFP64), PA1 como OPAMP1_VINP y PA2 como ADC1_IN3. Antes del cambio, HAL_ADC/HAL_OPAMP estaban deshabilitados y no había inicialización, calibración ni muestreo.

RM0316 Rev 10, tabla 26 y §18.3.4 confirman OPAMP1_VOUT → ADC1_IN3 en PA2. §18.4.1 selecciona PA1 con VP_SEL=11 (`OPAMP_NONINVERTINGINPUT_IO0`). No confundir ADC1_IN15/VREFOPAMP1, usado para referencia de calibración, con la salida de señal en canal 3.

La configuración implementada es seguidor de tensión (ganancia 1, realimentación interna), con PA1 y PA2 en modo analógico. No hace falta un cable PA1–PA2 ni una realimentación externa. PA2 también es salida física del opamp: no conectar allí otra fuente que la conduzca. El divisor debe alimentar PA1 para esta configuración.

El usuario confirmó que el divisor llega a PA1, usa 10 kΩ y 20 kΩ y no hay opamp externo. Se interpreta 10 kΩ desde sensor a PA1 y 20 kΩ desde PA1 a GND, consistente con reducir aproximadamente 5 V a 3.3 V. Falta confirmar qué canal del sensor está cableado.

## Conversión

DHAB S/118, hoja de datos versión 5 (15/03/2023), página 4:

| Salida | Rango | Sensibilidad a alimentación de 5 V | Offset sin corriente |
|---|---|---|---|
| Canal 1 | ±30 A | 0.0667 V/A | 2.5 V |
| Canal 2 | ±350 A | 0.0057 V/A | 2.5 V |

Para un divisor `k = R_inferior / (R_superior + R_inferior)` y opamp seguidor:

`I = ((ADC_promedio × VREF / 4095 / k) × 5 / V_sensor - 2.5) / sensibilidad`

El signo positivo sigue la flecha del sensor; no se ha asignado a carga o descarga de la batería.

Se usa `k=20/(10+20)=2/3`, alimentación del sensor 5 V y VREF ADC 3.3 V. Esto coloca cero corriente en 1.6667 V (aproximadamente 2068 cuentas). Confirmar orientación del divisor y tensiones reales antes de interpretar amperios. El fabricante especifica una carga mínima de 10 kΩ: para un divisor simple, revisar su resistencia total. El divisor también debe evaluarse con tolerancias y alimentación máxima, no únicamente con 5 V nominales.

`CURRENT_SENSOR_CHANNEL` queda en **0 (sin configurar)** hasta confirmar canal 1 o 2. Mientras tanto, se publican lecturas ADC pero `current_valid` queda falso y no se actualiza `pack_current_A`. No se selecciona automáticamente una sensibilidad.

## Implementación

`CurrentSensor_Init` configura/calibra OPAMP1 y ADC1 antes del scheduler. ADC de 12 bits, reloj síncrono HCLK/4 (18 MHz), canal 3 single-ended, muestreo de 181.5 ciclos y conversión disparada por software. La adquisición llama `CurrentSensor_Sample` una vez por iteración, nominalmente cada 10 ms. Reintentos/bloqueos del BQ alargan este período; no es muestreo independiente por timer/DMA.

Promedio móvil de las últimas cuatro muestras, usando solo las disponibles durante arranque. Una lectura inválida reinicia la ventana. No sustituye un filtro analógico antialias.

Telemetría bajo mutex:

- `current_adc_raw`: último código convertido.
- `current_adc_average`: promedio de códigos.
- `pack_current_A`: último valor válido en amperios, con signo.
- `current_tick_ms`: timestamp HAL del último valor válido.
- `current_valid`: validez del intento actual.
- `current_errors`: intentos sin amperios válidos, incluidos los que ocurren sin canal configurado.

Ante error ADC, código de rail o corriente fuera del rango nominal, se conserva el último valor válido y su timestamp. La verificación de rango instantáneo evita que el promedio oculte saturación. Esto no detecta todas las posibles desconexiones del sensor.

## Validación

Compilación Make/GCC; pruebas del módulo real con HAL simulado para los dos canales y el caso sin configurar: signo, promedio al inicio y ventana completa, timeout, rails y fuera de rango. Sin pruebas en placa ni verificación física del amplificador externo.

```sh
for channel in 0 1 2; do
  cc -std=c11 -Wall -Wextra -fsanitize=address,undefined \
    -DCURRENT_SENSOR_CHANNEL=$channel -Itests/stubs -ICore/Inc \
    tests/test_current_sensor.c Core/Src/current_sensor.c -o /tmp/bms-current-test
  /tmp/bms-current-test
done
```

La configuración manual debe trasladarse a CubeMX antes de regenerar: habilitar OPAMP1 follower en PA1 y ADC1 canal 3, y conservar los módulos HAL y fuentes agregadas al Makefile.

Referencias: PDFs locales aportados por el usuario, usados como documentación técnica, y [RM0316 de ST](https://www.st.com/resource/en/reference_manual/DM00043574.pdf).

## Selección según el borrador Formula SAE 2027 V0

Se revisó el PDF aportado, versión 0.0 del 21 julio de 2026. No exige un canal específico del DHAB.

- EV.3.3.1, página 95: límite de potencia 80 kW. Para corriente total de tracción se recomienda canal 2 (±350 A), condicionado a corriente máxima real, regeneración y transitorios. A 80 kW, 400 V corresponden a 200 A; 300 V a 266.7 A. Por debajo de 228.6 V, 80 kW requieren más de 350 A. Canal 1 no cubre 80 kW ni a 600 V (133.3 A).
- EV.8.7.1–3, página 111: BSPD independiente y no programable, disparo con frenada fuerte y corriente equivalente a 5 kW a tensión nominal durante más de 0.5 s, y detección de entradas abiertas/cortocircuitadas. Canal 1 puede ofrecer mejor resolución alrededor de ese umbral si 5000/Vnom queda dentro de su rango; no implica conformidad automática. El BSPD no puede depender de esta tarea del STM32.
- EV.3.2, página 95: el Energy Meter oficial sigue siendo obligatorio. Telemetría propia no lo sustituye.
- EV.3.4.1, página 95: evaluación de excedencias continuas de 100 ms o con promedio móvil de 500 ms. El promedio de cuatro muestras aquí implementado no es esa ventana reglamentaria.

Recomendación: canal 2 para esta telemetría, previa confirmación del cable conectado y de corriente máxima/voltaje mínimo del pack. El código mantiene canal 0 hasta confirmar el cableado: las reglas no permiten deducirlo.
