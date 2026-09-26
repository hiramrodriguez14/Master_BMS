#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { int unused; } SPI_HandleTypeDef;
typedef struct { int unused; } TIM_HandleTypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed, Alternate; } GPIO_InitTypeDef;
#define GPIOA 0
#define GPIOB 1
#define GPIO_PIN_4 16
#define GPIO_PIN_5 32
#define GPIO_PIN_6 64
#define GPIO_PIN_7 128
#define GPIO_MODE_OUTPUT_PP 0
#define GPIO_MODE_AF_PP 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_HIGH 1
#define GPIO_AF5_SPI1 5
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define BQ_SPI_READY_GPIO_Port GPIOB
#define BQ_SPI_READY_Pin 4
#define BQ79600CS_GPIO_Port GPIOA
#define BQ79600CS_Pin GPIO_PIN_4
#define CHARGE_PWR_SENSE_GPIO_Port GPIOB
#define CHARGE_PWR_SENSE_Pin 128
#define READY_PWR_SENSE_GPIO_Port GPIOB
#define READY_PWR_SENSE_Pin 32
#define __HAL_SPI_DISABLE(x) ((void)(x))
#define __HAL_SPI_ENABLE(x) ((void)(x))
void HAL_GPIO_Init(int port, GPIO_InitTypeDef *cfg);
void HAL_GPIO_WritePin(int port, int pin, int state);
int HAL_GPIO_ReadPin(int port, int pin);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *, uint8_t *, uint8_t *, uint16_t, uint32_t);
#include "analog_hal.h"
#endif
