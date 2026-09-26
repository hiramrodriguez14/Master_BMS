#ifndef BMS_CONFIG_H
#define BMS_CONFIG_H

/* Stack monitors only; the BQ79600 bridge has address zero. */
#ifndef TOTALBOARDS
#define TOTALBOARDS 8
#endif
#define ACTIVECHANNELS 14
#define GPIO_TEMPERATURES_PER_BQ 8U
#define TOTAL_TEMPERATURES (TOTALBOARDS * GPIO_TEMPERATURES_PER_BQ)
/* NTC to GND, external pull-up to TSREF. Populate from the actual BOM.
 * Zero values deliberately disable Celsius conversion, not raw acquisition. */
#ifndef BMS_NTC_R0_OHM
#define BMS_NTC_R0_OHM 0.0f
#endif
#ifndef BMS_NTC_T0_C
#define BMS_NTC_T0_C 25.0f
#endif
#ifndef BMS_NTC_BETA_K
#define BMS_NTC_BETA_K 0.0f
#endif
#ifndef BMS_NTC_PULLUP_OHM
#define BMS_NTC_PULLUP_OHM 0.0f
#endif
#define MAX_TEMP 40.0f
#define MIN_TEMP 10.0f
#define TOTAL_CELLS (TOTALBOARDS * ACTIVECHANNELS)
#define DATA_ACQUISITION_PERIOD_MS 5U
#if TOTALBOARDS < 1 || TOTALBOARDS > 63 || ACTIVECHANNELS < 6 || ACTIVECHANNELS > 16
#error Invalid BQ79616 stack configuration
#endif
#endif
