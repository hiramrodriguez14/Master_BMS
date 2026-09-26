#include "data_acquisition.h"
#include "current_sensor.h"
#include "FreeRTOS.h"
#include "task.h"

void acquiredata(void *argument)
{
    Telemetry_t *telemetry = argument;
    bool initialized = false;
    unsigned consecutive_errors = 0;
    TickType_t last = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(DATA_ACQUISITION_PERIOD_MS);
    configASSERT(telemetry != NULL && telemetryMutex != NULL && period > 0);

    for (;;) {
        CurrentSensor_Sample(telemetry);
        HAL_StatusTypeDef result = HAL_OK;
        if (!initialized) {
            result = BQ79600_WakeUp(TOTALBOARDS, false);
            if (result == HAL_OK) result = SpiAutoAddress(TOTALBOARDS);
            if (result == HAL_OK) result = BQ79616_StartADC();
            initialized = result == HAL_OK;
            last = xTaskGetTickCount();
        }
        if (initialized) result = stackVoltageRead(&telemetryMutex, telemetry);
        if (result == HAL_OK) {
            result = stackTemperatureRead(&telemetryMutex, telemetry);
        } else {
            /* No temperature read after a failed voltage transaction/initialization. */
            osMutexAcquire(telemetryMutex, osWaitForever);
            telemetry->voltage_valid = false;
            invalidateTemperatures(telemetry);
            osMutexRelease(telemetryMutex);
        }
        if (result != HAL_OK) {
            osMutexAcquire(telemetryMutex, osWaitForever);
            telemetry->acquisition_errors++;
            osMutexRelease(telemetryMutex);
            /* Allow pending daisy-chain replies to finish before clearing. */
            osDelay(10);
            SpiClear();
            if (++consecutive_errors >= 3U) initialized = false;
            if (!initialized) osDelay(100);
        }
        else {
            consecutive_errors = 0;
        }
        /* Resynchronize after overruns instead of issuing catch-up bursts. */
        if ((TickType_t)(xTaskGetTickCount() - last) >= period)
            last = xTaskGetTickCount();
        vTaskDelayUntil(&last, period);
    }
}
