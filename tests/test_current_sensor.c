#include "current_sensor.h"
#include "data_acquisition.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
osMutexId_t telemetryMutex=(void *)1;
static unsigned raw=2068,tick=100;
static HAL_StatusTypeDef adc_status=HAL_OK;
void HAL_GPIO_Init(int p,GPIO_InitTypeDef *c) {(void)p;assert(c->Pin==(GPIO_PIN_1|GPIO_PIN_2));}
void HAL_Delay(uint32_t t) {tick+=t;}
uint32_t HAL_GetTick(void) {return tick++;}
int osMutexAcquire(osMutexId_t m,uint32_t t) {(void)m;(void)t;return osOK;}
int osMutexRelease(osMutexId_t m) {(void)m;return osOK;}
HAL_StatusTypeDef HAL_OPAMP_Init(OPAMP_HandleTypeDef *h) {assert(h->Init.Mode==OPAMP_FOLLOWER_MODE);return HAL_OK;}
HAL_StatusTypeDef HAL_OPAMP_SelfCalibrate(OPAMP_HandleTypeDef *h) {(void)h;return HAL_OK;}
HAL_StatusTypeDef HAL_OPAMP_Start(OPAMP_HandleTypeDef *h) {(void)h;return HAL_OK;}
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *h) {assert(!h->Init.ContinuousConvMode);return HAL_OK;}
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *h,ADC_ChannelConfTypeDef *c) {(void)h;assert(c->Channel==3);return HAL_OK;}
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *h,uint32_t d) {(void)h;(void)d;return HAL_OK;}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *h) {(void)h;return adc_status;}
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *h,uint32_t t) {(void)h;assert(t==2);return adc_status;}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h) {(void)h;return raw;}
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *h) {(void)h;return HAL_OK;}
int main(void)
{
 Telemetry_t t={0};
 assert(CurrentSensor_Init()==HAL_OK);
 CurrentSensor_Sample(&t);
 assert(t.current_adc_raw==2068 && t.current_adc_average==2068);
 if(CURRENT_SENSOR_CHANNEL==0) {assert(!t.current_valid);puts("PASS: unconfigured channel cannot publish amperes");return 0;}
 assert(t.current_valid && fabsf(t.pack_current_A)<0.11f);
 raw=2500;CurrentSensor_Sample(&t);assert(t.current_adc_average==2284);
 raw=2600;CurrentSensor_Sample(&t);
 raw=2700;CurrentSensor_Sample(&t);assert(t.current_adc_average==2467);
 raw=2800;CurrentSensor_Sample(&t);assert(t.current_adc_average==2650);
 assert(t.pack_current_A>0);
 float saved=t.pack_current_A;uint32_t stamp=t.current_tick_ms;
 adc_status=HAL_TIMEOUT;CurrentSensor_Sample(&t);assert(!t.current_valid);
 assert(t.pack_current_A==saved && t.current_tick_ms==stamp);
 adc_status=HAL_OK;raw=1200;CurrentSensor_Sample(&t);
 assert(t.current_valid && t.current_adc_average==1200 && t.pack_current_A<0);
 raw=4095;CurrentSensor_Sample(&t);assert(!t.current_valid);
 raw=2048;CurrentSensor_Sample(&t);assert(t.current_valid && t.current_adc_average==2048);
 raw=4000;CurrentSensor_Sample(&t);assert(!t.current_valid);
 puts("PASS: ADC route, signed current, startup average, rolling window, timeout, rails, out-of-range");
}
