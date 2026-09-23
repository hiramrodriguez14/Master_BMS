#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include <stdint.h>
#include <stdbool.h>
#include "bq79600.h"
#define ACTIVE_CHANNELS 14
typedef struct{
  uint32_t tick_ms;

  float pack_voltage_mV;
  float pack_current_A;
  float pack_temp_C;
  float cell_voltage[ACTIVE_CHANNELS];
  float min_cell_voltage_mV;
  float max_cell_voltage_mV;
  float avg_cell_voltage_mV;

  uint8_t soc_percent;
  uint8_t fault_flags;
  bool charge_pwr_sense;
  bool ready_pwr_sense;
}Telemetry_t;
#endif //__TELEMETRY_H__
