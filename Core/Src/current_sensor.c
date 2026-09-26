#include "current_sensor.h"
#include "data_acquisition.h"
#include <math.h>
#include <string.h>

static OPAMP_HandleTypeDef hopamp1;
static ADC_HandleTypeDef hadc1;
static uint16_t history[4];
static uint32_t sum;
static unsigned count, next;
static bool initialized;

HAL_StatusTypeDef CurrentSensor_Init(void)
{
    initialized = false;
    memset(history, 0, sizeof(history));
    sum = count = next = 0;
    GPIO_InitTypeDef pins = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_ADC12_CLK_ENABLE();
    pins.Pin = GPIO_PIN_1 | GPIO_PIN_2;
    pins.Mode = GPIO_MODE_ANALOG;
    pins.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &pins);

    /* PA1 -> OPAMP1 follower -> PA2/ADC1_IN3. PA2 must not be driven externally. */
    hopamp1.Instance = OPAMP1;
    hopamp1.Init.Mode = OPAMP_FOLLOWER_MODE;
    hopamp1.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO0;
    hopamp1.Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp1.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
    if (HAL_OPAMP_Init(&hopamp1) != HAL_OK ||
        HAL_OPAMP_SelfCalibrate(&hopamp1) != HAL_OK ||
        HAL_OPAMP_Start(&hopamp1) != HAL_OK) return HAL_ERROR;
    HAL_Delay(1);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4; /* HCLK/4 = 18 MHz */
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) return HAL_ERROR;
    ADC_ChannelConfTypeDef channel = {0};
    channel.Channel = ADC_CHANNEL_3; /* Not ADC_CHANNEL_VOPAMP1 (calibration reference). */
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_181CYCLES_5;
    channel.SingleDiff = ADC_SINGLE_ENDED;
    channel.OffsetNumber = ADC_OFFSET_NONE;
    if (HAL_ADC_ConfigChannel(&hadc1, &channel) != HAL_OK ||
        HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
        return HAL_ERROR;
    initialized = true;
    return HAL_OK;
}

void CurrentSensor_Sample(Telemetry_t *telemetry)
{
    uint16_t raw = 0;
    HAL_StatusTypeDef status = initialized ? HAL_ADC_Start(&hadc1) : HAL_ERROR;
    if (status == HAL_OK) {
        status = HAL_ADC_PollForConversion(&hadc1, 2);
        if (status == HAL_OK) raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
        HAL_StatusTypeDef stopped = HAL_ADC_Stop(&hadc1);
        if (status == HAL_OK) status = stopped;
    }
    bool valid = false;
    float amps = 0.0f, averaged = 0.0f;
    if (status == HAL_OK && raw != 0U && raw != 4095U) {
        sum -= history[next];
        history[next] = raw;
        sum += raw;
        next = (next + 1U) % 4U;
        if (count < 4U) ++count;
        averaged = (float)sum / count;
        /* Sensor output is ratiometric in both offset and sensitivity. */
        float sensor_v = averaged * CURRENT_SENSOR_ADC_VREF_V / 4095.0f /
                         CURRENT_SENSOR_DIVIDER_RATIO;
        float normalized_v = sensor_v * 5.0f / CURRENT_SENSOR_SUPPLY_V;
        float sensitivity = CURRENT_SENSOR_CHANNEL == 1U ? 0.0667f : 0.0057f;
        amps = (normalized_v - 2.5f) / sensitivity;
        float limit = CURRENT_SENSOR_CHANNEL == 1U ? 30.0f : 350.0f;
        /* Check instantaneous range too: averaging must not hide clipping. */
        float instantaneous_v = raw * CURRENT_SENSOR_ADC_VREF_V / 4095.0f /
                               CURRENT_SENSOR_DIVIDER_RATIO * 5.0f / CURRENT_SENSOR_SUPPLY_V;
        float instantaneous_a = (instantaneous_v - 2.5f) / sensitivity;
        valid = (CURRENT_SENSOR_CHANNEL == 1U || CURRENT_SENSOR_CHANNEL == 2U) &&
                fabsf(amps) <= limit && fabsf(instantaneous_a) <= limit;
    } else {
        memset(history, 0, sizeof(history));
        sum = count = next = 0;
    }
    if (!valid) {
        /* Never carry invalid samples into the next valid averaging window. */
        memset(history, 0, sizeof(history));
        sum = count = next = 0;
    }
    if (osMutexAcquire(telemetryMutex, osWaitForever) != osOK) return;
    telemetry->current_valid = valid;
    if (status == HAL_OK) {
        telemetry->current_adc_raw = raw;
        telemetry->current_adc_average = averaged;
    }
    if (valid) {
        telemetry->pack_current_A = amps;
        telemetry->current_tick_ms = HAL_GetTick();
    } else {
        telemetry->current_errors++;
    }
    osMutexRelease(telemetryMutex);
}
