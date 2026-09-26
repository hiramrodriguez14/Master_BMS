#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H
#include <main.h>
#include "telemetry.h"

/* DHAB S/118 channel 1: 30 A; channel 2: 350 A. Zero means unconfigured. */
#ifndef CURRENT_SENSOR_CHANNEL
#define CURRENT_SENSOR_CHANNEL 0U
#endif
#define CURRENT_SENSOR_DIVIDER_RATIO (20000.0f / (10000.0f + 20000.0f)) /* 10k sensor->PA1, 20k PA1->GND. */
#define CURRENT_SENSOR_SUPPLY_V 5.0f
#define CURRENT_SENSOR_ADC_VREF_V 3.3f

HAL_StatusTypeDef CurrentSensor_Init(void);
void CurrentSensor_Sample(Telemetry_t *telemetry);
#endif
