#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include <stdint.h>
#include <stdbool.h>
#include "bms_config.h"
typedef struct{
  uint32_t tick_ms; /* Last successful voltage sample, HAL milliseconds. */
  uint32_t acquisition_errors;
  bool voltage_valid;
  bool current_valid;
  uint32_t current_tick_ms; /* Last valid current sample, independent of BQ voltage. */
  uint32_t current_errors;
  uint16_t current_adc_raw;
  float current_adc_average;
  bool temperature_valid; /* All configured GPIO temperatures valid. */
  uint32_t temperature_tick_ms; /* Last fully valid temperature scan. */
  uint32_t temperature_scan_tick_ms; /* Last CRC-valid raw GPIO/TSREF scan. */
  uint32_t temperature_errors;
  uint16_t tsref_adc_raw[TOTALBOARDS];
  uint16_t gpio_adc_raw[TOTALBOARDS][GPIO_TEMPERATURES_PER_BQ];
  float gpio_temperature_C[TOTALBOARDS][GPIO_TEMPERATURES_PER_BQ];
  bool gpio_temperature_valid[TOTALBOARDS][GPIO_TEMPERATURES_PER_BQ];
  float min_temperature_C;
  float max_temperature_C;

  float pack_voltage_mV;
  float pack_current_A;
  float pack_temp_C; /* Maximum GPIO temperature of last fully valid scan. */
  float cell_voltage[TOTAL_CELLS];
  float min_cell_voltage_mV;
  float max_cell_voltage_mV;
  float avg_cell_voltage_mV;

  uint8_t soc_percent;
  uint8_t fault_flags;
  bool charge_pwr_sense;
  bool ready_pwr_sense;
}Telemetry_t;
#endif //__TELEMETRY_H__
