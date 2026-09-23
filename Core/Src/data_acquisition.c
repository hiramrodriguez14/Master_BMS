#include "data_acquisition.h"
#include "telemetry.h"
void acquiredata (void* argument){
  //set the task to sample every 10 ms so 100Hz
  //get cell voltages with BQ driver, need to know how many cells there will be
   HAL_StatusTypeDef status;
   status  = stackVoltageRead(ACTIVECHANNELS);
   if(status == HAL_OK){
    printf("VOLTAGE_READ_GOOD\r\n");
   } else {
    printf("VOLTAGE READ ERROR %d\r\n",status);
   }
  //get temperatures, need to modify bq driver
  //read ADC, make a 4 sample moving average
}
