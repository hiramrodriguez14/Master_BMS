/**
  ******************************************************************************
  * @file           : bq79600.c
  * @brief          : BQ79600/BQ79616 function implementations
  ******************************************************************************
  */

#include <bq79600.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
/* Single-owner driver: only the acquisition task may access SPI1. */
static uint16_t crc;
static HAL_StatusTypeDef status;
static uint32_t timeout;
static uint8_t tx_data[8];
static uint8_t rx_data[RX_BUFFER_SIZE];

extern SPI_HandleTypeDef hspi1;
extern void Delay_us(uint32_t us);

/* BQ79600 requires MOSI high between command frames. */
static void mosiMode(bool idle)
{
    GPIO_InitTypeDef pin = {0};
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    pin.Pin = GPIO_PIN_7;
    pin.Mode = idle ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_AF_PP;
    pin.Pull = GPIO_NOPULL;
    pin.Speed = GPIO_SPEED_FREQ_HIGH;
    pin.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &pin);
}

// CRC16 TABLE
// ITU_T polynomial: x^16 + x^15 + x^2 + 1
const uint16_t crc16_table[256] = {0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301,
                                   0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1,
                                   0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81,
                                   0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
                                   0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00,
                                   0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1,
                                   0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380,
                                   0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141,
                                   0x3300, 0xF3C1, 0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501,
                                   0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0,
                                   0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881,
                                   0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
                                   0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401,
                                   0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1,
                                   0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0, 0x6180,
                                   0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740,
                                   0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01,
                                   0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1,
                                   0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80,
                                   0xBA41, 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
                                   0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200,
                                   0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1,
                                   0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780,
                                   0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41,
                                   0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901,
                                   0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1,
                                   0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80,
                                   0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
                                   0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};

/**
 * @brief  Calculates CRC-16-IBM for BQ79600 communication
 * @param  data: Buffer containing the data
 * @param  length: Length of data
 * @retval uint16_t: Calculated CRC value
 */
uint16_t SpiCRC16(uint8_t* pBuf, int sendLen)
{
    uint16_t wCRC = 0xFFFF;
    int i;

    for (i = 0; i < sendLen; i++)
    {
        wCRC ^= (uint16_t)(pBuf[i] & 0x00FF);
        wCRC = crc16_table[wCRC & 0x00FF] ^ (wCRC >> 8);
    }

    //printf("CRC16 calculated: 0x%04X\n", wCRC);

    return wCRC;
}

/**
 * @brief  Temporarily disable SPI1 to control MOSI pin as GPIO
 * @retval None
 */
void SPI1_DisableForGPIO(void)
{
  // Disable SPI1
  __HAL_SPI_DISABLE(&hspi1);

  // Configure MOSI pin (PA7) as GPIO output
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_7;        // PA7 is SPI1_MOSI on most STM32F4 boards
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief  Restore SPI1 configuration for normal operation
 * @retval None
 */
void SPI1_RestoreFromGPIO(void)
{
  // Re-initialize SPI1 pins to their original function
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // SCK pin (PA5)
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // MISO pin (PA6)
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // MOSI pin (PA7)
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Re-enable SPI1
  __HAL_SPI_ENABLE(&hspi1);
  mosiMode(true);
}

/*
 *
 * @brief  Performs the BQ79600-Q1 wakeup sequence
 * @param  num_stacked_devices: Number of stacked BQ79616-Q1 devices
 * @param  need_double_wake: Set to true if device was previously shut down using SHUTDOWN ping
 * @retval HAL status
*/
HAL_StatusTypeDef BQ79600_WakeUp(uint8_t num_stacked_devices, bool need_double_wake)
{

  // 1. Send WAKE ping - begin by disabling SPI to control MOSI directly
  SPI1_DisableForGPIO();

  // Configure NSS pin (PA4) as GPIO output
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_4;  // NSS pin
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // If device was shut down with SHUTDOWN ping, we need two WAKE pings
  if (need_double_wake) {
    // First WAKE ping
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // Hold nCS low
    Delay_us(2);  // Wait 2us

    // Pull MOSI low for 2.75ms (tHLD_WAKE)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    Delay_us(BQ79600_WAKE_PING_TIME_US);  // 2.75ms
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);

    Delay_us(2);  // Wait 2us
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  // Bring nCS back high

    // Wait for first wake ping to process (3.5ms)
    Delay_us(BQ79600_WAKE_SETUP_TIME_US);  // 3.5ms
  }

  // Send (second) WAKE ping
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // Hold nCS low
  Delay_us(2);  // Wait 2us

  // Pull MOSI low for 2.75ms (tHLD_WAKE)
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
  Delay_us(BQ79600_WAKE_PING_TIME_US);  // 2.75ms
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);

  Delay_us(2);  // Wait 2us
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  // Bring nCS back high

  // Restore SPI configuration
  SPI1_RestoreFromGPIO();

  // 2. Wait for tSU(WAKE_SHUT) to allow BQ79600-Q1 to enter ACTIVE mode (3.5ms)
  HAL_Delay(4);

  // 4. Send a single device write to set CONTROL1[SEND_WAKE]=1, which wakes up all stacked devices
  tx_data[0] = 0x90;  // Single device 1 byte write
  tx_data[1] = 0x00;  // Device address
  tx_data[2] = 0x03;  // MSB register address
  tx_data[3] = 0x09;  // LSB register address
  tx_data[4] = 0x20;  // 00100000 (enable SEND_WAKE)

  HAL_StatusTypeDef result = SpiWrite(5);
  if (result != HAL_OK) return result;
  /* Each stack device must propagate WAKE and enter ACTIVE. */
  HAL_Delay((num_stacked_devices * (BQ79600_WAKE_TONE_TIME_US +
             BQ79600_ACTIVE_MODE_TIME_US) + 999U) / 1000U);
  return HAL_OK;
}

/**
 * @brief  Auto address the bq79600s. Set all devices to stack mode, set the highest device as top of stack, synchronize the DLL.
 * @param  None
 * @retval None
 */
static HAL_StatusTypeDef writeByte(uint8_t command, uint8_t device,
                                   uint16_t reg, uint8_t value)
{
    int length = 0;
    tx_data[length++] = command;
    if (command == CMD_SINGLE_DEV_WRITE) tx_data[length++] = device;
    tx_data[length++] = reg >> 8;
    tx_data[length++] = reg & 0xFF;
    tx_data[length++] = value;
    return SpiWrite(length);
}

HAL_StatusTypeDef SpiAutoAddress(uint8_t numStackedDevices)
{
    if (numStackedDevices != TOTALBOARDS) return HAL_ERROR;
    HAL_StatusTypeDef result;
    for (uint16_t reg = 0x0343; reg <= 0x034A; ++reg) {
        result = writeByte(CMD_STACK_WRITE, 0, reg, 0);
        if (result != HAL_OK) return result;
    }
    result = writeByte(CMD_BROADCAST_WRITE, 0, REG_CONTROL1, 1);
    if (result != HAL_OK) return result;
    /* Address zero belongs to the bridge, monitors are 1..TOTALBOARDS. */
    for (uint8_t address = 0; address <= numStackedDevices; ++address) {
        result = writeByte(CMD_BROADCAST_WRITE, 0, REG_DIR0_ADDR, address);
        if (result != HAL_OK) return result;
    }
    result = writeByte(CMD_BROADCAST_WRITE, 0, REG_COMM_CTRL, 2);
    if (result != HAL_OK) return result;
    result = writeByte(CMD_SINGLE_DEV_WRITE, numStackedDevices, REG_COMM_CTRL, 3);
    if (result != HAL_OK) return result;
    for (uint16_t reg = 0x0343; reg <= 0x034A; ++reg) {
        tx_data[0] = CMD_STACK_READ;
        tx_data[1] = reg >> 8;
        tx_data[2] = reg & 0xFF;
        tx_data[3] = 0;
        result = SpiRead(4, 7 * numStackedDevices);
        if (result != HAL_OK) return result;
    }
    for (uint8_t address = 0; address <= numStackedDevices; ++address) {
        tx_data[0] = CMD_SINGLE_DEV_READ;
        tx_data[1] = address;
        tx_data[2] = REG_DIR0_ADDR >> 8;
        tx_data[3] = REG_DIR0_ADDR & 0xFF;
        tx_data[4] = 0;
        result = SpiRead(5, 7);
        if (result != HAL_OK) return result;
        if (rx_data[4] != address) return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef SpiWrite(int sendLen)
{
	  if (sendLen < 1 || sendLen > (int)sizeof(tx_data) - 2) return HAL_ERROR;
	  crc = SpiCRC16(tx_data, sendLen);
	  tx_data[sendLen] = crc & 0xFF;
	  tx_data[sendLen + 1] = (crc >> 8) & 0xFF;

	  //Check if SPI_READY is high, with timeout
	  timeout = HAL_GetTick();  // 100ms timeout
	  while (HAL_GPIO_ReadPin(BQ_SPI_READY_GPIO_Port, BQ_SPI_READY_Pin) != GPIO_PIN_SET) {
	    if ((uint32_t)(HAL_GetTick() - timeout) >= 100U) {
	      return HAL_TIMEOUT;
	    }
	    Delay_us(100);
	  }

	  mosiMode(false);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

	  Delay_us(1); //t9

	  // Send the command
	  status = HAL_SPI_Transmit(&hspi1, tx_data, sendLen + 2, 100);

	  // Pull nCS high
	  Delay_us(1); //t10
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	  mosiMode(true);

	  Delay_us(1);

	  if (status != HAL_OK) {
	    return status;
	  }

	  return HAL_OK;
}

HAL_StatusTypeDef SpiRead(int sendLen, int returnLen)
{
    uint8_t dummy[128];
    if (returnLen < 7 || returnLen > (int)sizeof(rx_data) ||
        (sendLen != 4 && sendLen != 5)) return HAL_ERROR;
    const uint8_t command = tx_data[0];
    const uint8_t device = sendLen == 5 ? tx_data[1] : 0;
    const uint16_t reg = ((uint16_t)tx_data[sendLen - 3] << 8) |
                         tx_data[sendLen - 2];
    const unsigned payload = (unsigned)tx_data[sendLen - 1] + 1U;
    const unsigned frameSize = payload + 6U;
    if ((unsigned)returnLen % frameSize != 0U) return HAL_ERROR;
    HAL_StatusTypeDef result = SpiWrite(sendLen);
    if (result != HAL_OK) return result;
    memset(dummy, 0xFF, sizeof(dummy));
    /* Allow SPI_RDY to go low following the read command. */
    Delay_us(5);
    uint32_t start = HAL_GetTick();
    const uint32_t waitMs = 2U + ((TOTALBOARDS - 1U) * 6U +
                                      (unsigned)returnLen * 10U + 100U) / 1000U;
    for (int offset = 0; offset < returnLen;) {
        while (HAL_GPIO_ReadPin(BQ_SPI_READY_GPIO_Port, BQ_SPI_READY_Pin) != GPIO_PIN_SET) {
            if ((uint32_t)(HAL_GetTick() - start) >= waitMs) return HAL_TIMEOUT;
            Delay_us(5);
        }
        int count = returnLen - offset;
        if (count > 128) count = 128;
        mosiMode(false);
        HAL_GPIO_WritePin(BQ79600CS_GPIO_Port, BQ79600CS_Pin, GPIO_PIN_RESET);
        Delay_us(1);
        result = HAL_SPI_TransmitReceive(&hspi1, dummy, &rx_data[offset], count, 5);
        Delay_us(1);
        HAL_GPIO_WritePin(BQ79600CS_GPIO_Port, BQ79600CS_Pin, GPIO_PIN_SET);
        mosiMode(true);
        if (result != HAL_OK) return result;
        offset += count;
        Delay_us(5);
    }
    bool seen[TOTALBOARDS + 1] = {false};
    for (unsigned offset = 0; offset < (unsigned)returnLen; offset += frameSize) {
        uint8_t *frame = &rx_data[offset];
        if (frame[0] != payload - 1U || frame[2] != (reg >> 8) ||
            frame[3] != (reg & 0xFF) || SpiCRC16(frame, frameSize) != 0)
            return HAL_ERROR;
        if (command == CMD_SINGLE_DEV_READ) {
            if (frame[1] != device) return HAL_ERROR;
        } else {
            if (frame[1] == 0 || frame[1] > TOTALBOARDS || seen[frame[1]])
                return HAL_ERROR;
            seen[frame[1]] = true;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef SpiClear(){

	tx_data[0] = 0x00;

	mosiMode(false);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // Hold nCS low
	Delay_us(1);
	status = HAL_SPI_Transmit(&hspi1, tx_data, 1, 100);
	Delay_us(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);  // Pull nCS high
	mosiMode(true);

	if (status != HAL_OK) {
		    return status;
		  }

	return HAL_OK;
}

HAL_StatusTypeDef BQ79616_StartADC(void)
{
    /* ACTIVE_CELL encodes 6S as zero, 14S as eight. */
    HAL_StatusTypeDef result = writeByte(CMD_STACK_WRITE, 0, 0x0003,
                                         ACTIVECHANNELS - 6);
    if (result != HAL_OK) return result;
    /* ADC-only inputs, two GPIOs per register. No internal weak pulls. */
    for (uint16_t reg = 0x000E; reg <= 0x0011; ++reg) {
        result = writeByte(CMD_STACK_WRITE, 0, reg, 0x12);
        if (result != HAL_OK) return result;
    }
    result = writeByte(CMD_STACK_WRITE, 0, REG_CONTROL2, 0x01); /* TSREF_EN */
    if (result != HAL_OK) return result;
    /* TSREF ramp is 6 ms (10-90%, 1 uF); allow 20 ms before ADC start.
     * Revisit for the actual TSREF capacitance and external RC network. */
    HAL_Delay(20);
    result = writeByte(CMD_STACK_WRITE, 0, 0x030D, 0x06);
    if (result == HAL_OK) HAL_Delay(3); /* Eight main round robins: 8 * 192 us nominal. */
    return result;
}

HAL_StatusTypeDef stackVoltageRead(osMutexId_t *mutex, Telemetry_t *telemetry)
{
    if (mutex == NULL || *mutex == NULL || telemetry == NULL) return HAL_ERROR;
    int32_t cells[TOTAL_CELLS];
    int32_t min = INT32_MAX, max = INT32_MIN, pack = 0;
    tx_data[0] = CMD_STACK_READ;
    tx_data[1] = VCELL_START_HI >> 8;
    tx_data[2] = VCELL_START_HI & 0xFF;
    tx_data[3] = CELL_BYTES_PER_BQ - 1;
    HAL_StatusTypeDef result = SpiRead(4, TOTAL_RESPONSE);
    if (result != HAL_OK) return result;
    for (unsigned frame = 0; frame < TOTALBOARDS; ++frame) {
        const uint8_t *response = &rx_data[frame * RESPONSE_BYTES_BQ];
        /* Responses arrive top first; map by address, not arrival order. */
        unsigned board = response[1] - 1U;
        for (unsigned regCell = 0; regCell < ACTIVECHANNELS; ++regCell) {
            const uint8_t *raw = &response[4 + 2 * regCell];
            if (raw[0] == 0x80 && raw[1] == 0) return HAL_ERROR;
            int32_t mv = convert_adc_to_voltage(raw[0], raw[1]);
            cells[board * ACTIVECHANNELS + ACTIVECHANNELS - 1 - regCell] = mv;
            pack += mv;
            if (mv < min) min = mv;
            if (mv > max) max = mv;
        }
    }
    if (osMutexAcquire(*mutex, osWaitForever) != osOK) return HAL_ERROR;
    for (unsigned cell = 0; cell < TOTAL_CELLS; ++cell)
        telemetry->cell_voltage[cell] = cells[cell];
    telemetry->pack_voltage_mV = pack;
    telemetry->min_cell_voltage_mV = min;
    telemetry->max_cell_voltage_mV = max;
    telemetry->avg_cell_voltage_mV = (float)pack / TOTAL_CELLS;
    telemetry->tick_ms = HAL_GetTick();
    telemetry->voltage_valid = true;
    telemetry->charge_pwr_sense = HAL_GPIO_ReadPin(CHARGE_PWR_SENSE_GPIO_Port,
                                                  CHARGE_PWR_SENSE_Pin) == GPIO_PIN_SET;
    telemetry->ready_pwr_sense = HAL_GPIO_ReadPin(READY_PWR_SENSE_GPIO_Port,
                                                 READY_PWR_SENSE_Pin) == GPIO_PIN_SET;
    osMutexRelease(*mutex);
    return HAL_OK;
}

bool convert_gpio_to_temperature(uint16_t gpio_raw, uint16_t tsref_raw, float *celsius)
{
    if (celsius == NULL || gpio_raw == 0 || gpio_raw >= 0x8000U ||
        tsref_raw == 0 || tsref_raw >= 0x8000U ||
        BMS_NTC_R0_OHM <= 0.0f || BMS_NTC_PULLUP_OHM <= 0.0f ||
        BMS_NTC_BETA_K <= 0.0f || BMS_NTC_T0_C <= -273.15f)
        return false;
    /* Both ADC values are signed; GPIO and TSREF have DIFFERENT LSB weights. */
    float gpio_uv = gpio_raw * 152.59f;
    float reference_uv = tsref_raw * 169.54f;
    /* Reject near-rail values (short/open); 0.1% guard, not a full wire diagnostic. */
    float ratio = gpio_uv / reference_uv;
    if (!isfinite(ratio) || ratio <= 0.001f || ratio >= 0.999f) return false;
    float resistance = BMS_NTC_PULLUP_OHM * ratio / (1.0f - ratio);
    float inverse_kelvin = 1.0f / (BMS_NTC_T0_C + 273.15f) +
                          logf(resistance / BMS_NTC_R0_OHM) / BMS_NTC_BETA_K;
    if (!isfinite(inverse_kelvin) || inverse_kelvin <= 0.0f) return false;
    float result = 1.0f / inverse_kelvin - 273.15f;
    if (!isfinite(result)) return false;
    *celsius = result;
    return true;
}

void invalidateTemperatures(Telemetry_t *telemetry)
{
    telemetry->temperature_valid = false;
    memset(telemetry->gpio_temperature_valid, 0,
           sizeof(telemetry->gpio_temperature_valid));
    telemetry->temperature_errors++;
}

HAL_StatusTypeDef stackTemperatureRead(osMutexId_t *mutex, Telemetry_t *telemetry)
{
    if (mutex == NULL || *mutex == NULL || telemetry == NULL) return HAL_ERROR;
    tx_data[0] = CMD_STACK_READ;
    tx_data[1] = REG_TSREF_HI >> 8;
    tx_data[2] = REG_TSREF_HI & 0xFF;
    /* One contiguous read: TSREF then GPIO1..GPIO8, all high/low byte pairs. */
    tx_data[3] = TEMPERATURE_BYTES_PER_BQ - 1U;
    HAL_StatusTypeDef result = SpiRead(4, TOTAL_TEMPERATURE_RESPONSE);
    if (result != HAL_OK) {
        if (osMutexAcquire(*mutex, osWaitForever) == osOK) {
            invalidateTemperatures(telemetry);
            osMutexRelease(*mutex);
        }
        return result;
    }
    float temperatures[TOTALBOARDS][GPIO_TEMPERATURES_PER_BQ];
    bool valid[TOTALBOARDS][GPIO_TEMPERATURES_PER_BQ];
    uint16_t raw[TOTALBOARDS][GPIO_TEMPERATURES_PER_BQ];
    uint16_t references[TOTALBOARDS];
    bool all_valid = true;
    float min = FLT_MAX, max = -FLT_MAX;
    for (unsigned frame = 0; frame < TOTALBOARDS; ++frame) {
        const uint8_t *response = &rx_data[frame * TEMPERATURE_RESPONSE_BYTES_BQ];
        unsigned board = response[1] - 1U;
        references[board] = ((uint16_t)response[4] << 8) | response[5];
        for (unsigned gpio = 0; gpio < GPIO_TEMPERATURES_PER_BQ; ++gpio) {
            const uint8_t *value = &response[6U + gpio * 2U];
            raw[board][gpio] = ((uint16_t)value[0] << 8) | value[1];
            temperatures[board][gpio] = NAN;
            valid[board][gpio] = convert_gpio_to_temperature(raw[board][gpio],
                                    references[board], &temperatures[board][gpio]);
            if (!valid[board][gpio]) all_valid = false;
            else {
                if (temperatures[board][gpio] < min) min = temperatures[board][gpio];
                if (temperatures[board][gpio] > max) max = temperatures[board][gpio];
            }
        }
    }
    if (osMutexAcquire(*mutex, osWaitForever) != osOK) return HAL_ERROR;
    memcpy(telemetry->tsref_adc_raw, references, sizeof(references));
    memcpy(telemetry->gpio_adc_raw, raw, sizeof(raw));
    memcpy(telemetry->gpio_temperature_C, temperatures, sizeof(temperatures));
    memcpy(telemetry->gpio_temperature_valid, valid, sizeof(valid));
    telemetry->temperature_scan_tick_ms = HAL_GetTick();
    telemetry->temperature_valid = all_valid;
    if (all_valid) {
        telemetry->temperature_tick_ms = telemetry->temperature_scan_tick_ms;
        telemetry->min_temperature_C = min;
        telemetry->max_temperature_C = max;
        telemetry->pack_temp_C = max;
    } else {
        telemetry->temperature_errors++;
    }
    osMutexRelease(*mutex);
    /* Sensor/configuration faults do not require resetting the communication chain. */
    return HAL_OK;
}

int32_t convert_adc_to_voltage(uint8_t high_byte, uint8_t low_byte)
{
    int16_t raw = (int16_t)(((uint16_t)high_byte << 8) | low_byte);
    /* Signed 190.73 microvolts/LSB, result in millivolts. */
    return ((int32_t)raw * 19073) / 100000;
}

HAL_StatusTypeDef simpleBalancing(){
	//Set up active channels
	tx_data[0] = 0x90;
	tx_data[1] = 0x01;
	tx_data[2] = 0x00;
	tx_data[3] = 0x03;
	tx_data[4] = 0x0A;
	status = SpiWrite(5);

	if (status != HAL_OK) {
	    	return status;
	    }

	//choose channel
	tx_data[0] = 0x90;
	tx_data[1] = 0x01;
	tx_data[2] = 0x03;
	tx_data[3] = 0x26;
	tx_data[4] = 0x02;	//10 seconds balancing
	status = SpiWrite(5);

	if (status != HAL_OK) {
	    	return status;
	    }

	//Set up autobalancing
	tx_data[0] = 0x90;
	tx_data[1] = 0x01;
	tx_data[2] = 0x03;
	tx_data[3] = 0x2F;
	tx_data[4] = 0x01;
	status = SpiWrite(5);

	if (status != HAL_OK) {
	    	return status;
	    }

	//Start balacing
	tx_data[0] = 0x90;
	tx_data[1] = 0x01;
	tx_data[2] = 0x03;
	tx_data[3] = 0x2F;
	tx_data[4] = 0x02;
	status = SpiWrite(5);


	//Read if balancing started
	tx_data[0] = 0x80;
	tx_data[1] = 0x01;
	tx_data[2] = 0x05;
	tx_data[3] = 0x2B;
	tx_data[4] = 0x00;
	status = SpiRead(5, 6 + 1);

	uint8_t read = rx_data[4];

	printf("BAL_STAT %d \r\n", read);

	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}


