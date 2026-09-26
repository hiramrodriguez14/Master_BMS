#include "bq79600.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim4;
extern uint8_t tx_data[];
static uint8_t reply[RX_BUFFER_SIZE];
static unsigned cursor, transfers, tick;
static int ready = 1;
static HAL_StatusTypeDef tx_result, rx_result;
void HAL_GPIO_Init(int p, GPIO_InitTypeDef *c) {(void)p;(void)c;}
void HAL_GPIO_WritePin(int p,int n,int s) {(void)p;(void)n;(void)s;}
int HAL_GPIO_ReadPin(int p,int n) {(void)p;(void)n;return ready;}
uint32_t HAL_GetTick(void) {return tick++;}
void HAL_Delay(uint32_t ms) {tick += ms;}
void Delay_us(uint32_t us) {(void)us;}
int osMutexAcquire(osMutexId_t m,uint32_t t) {(void)m;(void)t;return osOK;}
int osMutexRelease(osMutexId_t m) {(void)m;return osOK;}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h,uint8_t *b,uint16_t n,uint32_t t)
{(void)h;(void)b;(void)n;(void)t;cursor=0;transfers=0;return tx_result;}
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *h,uint8_t *tx,uint8_t *rx,uint16_t n,uint32_t t)
{
 (void)h; assert(t != UINT32_MAX); assert(n <= 128); assert(cursor+n<=sizeof(reply));
 for(unsigned i=0;i<n;i++) assert(tx[i]==0xFF);
 memcpy(rx,reply+cursor,n);cursor+=n;transfers++;return rx_result;
}
static void crc_frame(uint8_t *f)
{uint16_t crc=SpiCRC16(f,RESPONSE_BYTES_BQ-2);f[RESPONSE_BYTES_BQ-2]=crc;f[RESPONSE_BYTES_BQ-1]=crc>>8;}
static void prepare(void)
{
 for(unsigned b=0;b<TOTALBOARDS;b++) {
  uint8_t *f=reply+b*RESPONSE_BYTES_BQ;
  unsigned addr=TOTALBOARDS-b;
  f[0]=CELL_BYTES_PER_BQ-1;f[1]=addr;f[2]=VCELL_START_HI>>8;f[3]=VCELL_START_HI&255;
  for(unsigned c=0;c<ACTIVECHANNELS;c++) {
   uint16_t raw=18000+addr*100+ACTIVECHANNELS-c;
   f[4+2*c]=raw>>8;f[5+2*c]=raw;
  }
  crc_frame(f);
 }
}
static void temperature_crc(uint8_t *f)
{
 uint16_t crc=SpiCRC16(f,TEMPERATURE_RESPONSE_BYTES_BQ-2);
 f[TEMPERATURE_RESPONSE_BYTES_BQ-2]=crc;f[TEMPERATURE_RESPONSE_BYTES_BQ-1]=crc>>8;
}
static void prepare_temperatures(void)
{
 for(unsigned b=0;b<TOTALBOARDS;b++) {
  uint8_t *f=reply+b*TEMPERATURE_RESPONSE_BYTES_BQ;
  unsigned addr=TOTALBOARDS-b;
  f[0]=TEMPERATURE_BYTES_PER_BQ-1;f[1]=addr;
  f[2]=REG_TSREF_HI>>8;f[3]=REG_TSREF_HI&255;
  f[4]=29491>>8;f[5]=29491&255;
  for(unsigned g=0;g<GPIO_TEMPERATURES_PER_BQ;g++) {
   uint16_t raw=16384+addr*100+g*10;
   f[6+2*g]=raw>>8;f[7+2*g]=raw;
  }
  temperature_crc(f);
 }
}
static void test_temperatures(osMutexId_t *mutex,Telemetry_t *data)
{
 float c=123;
 bool configured=BMS_NTC_R0_OHM>0 && BMS_NTC_BETA_K>0 && BMS_NTC_PULLUP_OHM>0;
 if(configured) {
  /* Test fixture only: 10k NTC, beta3950, 10k pull-up. Not hardware defaults. */
  assert(convert_gpio_to_temperature(16384,29491,&c) && fabsf(c-25)<0.02f);
  assert(convert_gpio_to_temperature(25256,29491,&c) && fabsf(c)<0.02f);
  assert(convert_gpio_to_temperature(6524,29491,&c) && fabsf(c-60)<0.02f);
 } else assert(!convert_gpio_to_temperature(16384,29491,&c));
 assert(!convert_gpio_to_temperature(0x8000,29491,&c));
 assert(!convert_gpio_to_temperature(10000,0x8000,&c));
 assert(!convert_gpio_to_temperature(0,29491,&c));
 assert(!convert_gpio_to_temperature(32767,29491,&c));
 ready=1;prepare_temperatures();
 assert(stackTemperatureRead(mutex,data)==HAL_OK);
 assert(transfers==(TOTAL_TEMPERATURE_RESPONSE+127)/128);
 assert(data->temperature_valid==configured);
 for(unsigned b=0;b<TOTALBOARDS;b++) for(unsigned g=0;g<8;g++) {
  assert(data->gpio_adc_raw[b][g]==16384+(b+1)*100+g*10);
  assert(data->tsref_adc_raw[b]==29491);
  assert(data->gpio_temperature_valid[b][g]==configured);
 }
 if(configured) {
  assert(data->pack_temp_C==data->max_temperature_C);
  assert(data->max_temperature_C==data->gpio_temperature_C[0][0]);
  assert(data->min_temperature_C==data->gpio_temperature_C[TOTALBOARDS-1][7]);
 }
 uint32_t stamp=data->temperature_tick_ms,scan=data->temperature_scan_tick_ms;
 uint16_t saved_raw=data->gpio_adc_raw[TOTALBOARDS-1][0];
 reply[6]^=1;assert(stackTemperatureRead(mutex,data)==HAL_ERROR);
 assert(!data->temperature_valid && data->temperature_tick_ms==stamp);
 assert(data->temperature_scan_tick_ms==scan);
 assert(data->gpio_adc_raw[TOTALBOARDS-1][0]==saved_raw);
 for(unsigned b=0;b<TOTALBOARDS;b++) for(unsigned g=0;g<8;g++)
  assert(!data->gpio_temperature_valid[b][g]);
 prepare_temperatures();reply[6]=0x80;reply[7]=0;temperature_crc(reply);
 assert(stackTemperatureRead(mutex,data)==HAL_OK);
 assert(!data->temperature_valid && !data->gpio_temperature_valid[TOTALBOARDS-1][0]);
 assert(isnan(data->gpio_temperature_C[TOTALBOARDS-1][0]));
 assert(data->gpio_temperature_valid[0][0]==configured);
 assert(data->temperature_tick_ms==stamp);
 prepare_temperatures();reply[4]=0;reply[5]=0;temperature_crc(reply);
 assert(stackTemperatureRead(mutex,data)==HAL_OK);
 for(unsigned g=0;g<8;g++) assert(!data->gpio_temperature_valid[TOTALBOARDS-1][g]);
 prepare_temperatures();tx_result=HAL_TIMEOUT;
 assert(stackTemperatureRead(mutex,data)==HAL_TIMEOUT);tx_result=HAL_OK;
 puts("PASS: GPIO/TSREF scales, NTC conversion, board/GPIO mapping, validity, stale timestamp, corrupt frame, missing sensor/reference");
}
int main(void)
{
 Telemetry_t data={0}, saved;
 osMutexId_t mutex=&data;
 uint8_t known[]={0xB0,0x00,0x03,0x0A};
 assert(SpiCRC16(known,4)==0x13A6);
 assert(convert_adc_to_voltage(0x4E,0x20)==3814);
 assert(convert_adc_to_voltage(0xFF,0xFF)==0);
 assert(convert_adc_to_voltage(0xD8,0xF0)==-1907);
 prepare();assert(stackVoltageRead(&mutex,&data)==HAL_OK);
 assert(transfers==(TOTAL_RESPONSE+127)/128 && cursor==TOTAL_RESPONSE && data.voltage_valid);
 for(unsigned b=0;b<TOTALBOARDS;b++) for(unsigned c=0;c<ACTIVECHANNELS;c++) {
  uint16_t raw=18000+(b+1)*100+c+1;
  assert(data.cell_voltage[b*ACTIVECHANNELS+c]==convert_adc_to_voltage(raw>>8,raw));
 }
 saved=data;reply[9]^=1;
 assert(stackVoltageRead(&mutex,&data)==HAL_ERROR);assert(memcmp(&data,&saved,sizeof(data))==0);
 prepare();reply[1]=0;crc_frame(reply);assert(stackVoltageRead(&mutex,&data)==HAL_ERROR);
 prepare();reply[1]=reply[RESPONSE_BYTES_BQ+1];crc_frame(reply);assert(stackVoltageRead(&mutex,&data)==HAL_ERROR);
 prepare();reply[4]=0x80;reply[5]=0;crc_frame(reply);assert(stackVoltageRead(&mutex,&data)==HAL_ERROR);
 prepare();reply[3]^=1;crc_frame(reply);assert(stackVoltageRead(&mutex,&data)==HAL_ERROR);
 prepare();tx_result=HAL_BUSY;assert(stackVoltageRead(&mutex,&data)==HAL_BUSY);tx_result=HAL_OK;
 rx_result=HAL_ERROR;assert(stackVoltageRead(&mutex,&data)==HAL_ERROR);rx_result=HAL_OK;
 ready=0;tick=UINT32_MAX-20;assert(stackVoltageRead(&mutex,&data)==HAL_TIMEOUT);
 assert(memcmp(&data,&saved,sizeof(data))==0);
 assert(SpiRead(4,TOTAL_RESPONSE+1)==HAL_ERROR);assert(SpiWrite(7)==HAL_ERROR);
 test_temperatures(&mutex,&data);
 puts("PASS: CRC, signed conversion, all-cell ordering, 128-byte chunks, corrupt/duplicate frames, sentinel, HAL errors, tick wrap, atomic publication");
}
