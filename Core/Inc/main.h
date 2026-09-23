/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define BQ79600CS_Pin GPIO_PIN_4
#define BQ79600CS_GPIO_Port GPIOA
#define BQ_NFAULT_Pin GPIO_PIN_1
#define BQ_NFAULT_GPIO_Port GPIOB
#define BQ_SPI_READY_Pin GPIO_PIN_2
#define BQ_SPI_READY_GPIO_Port GPIOB
#define SDCARD_CS_Pin GPIO_PIN_12
#define SDCARD_CS_GPIO_Port GPIOB
#define RED_Pin GPIO_PIN_8
#define RED_GPIO_Port GPIOA
#define BLUE_Pin GPIO_PIN_9
#define BLUE_GPIO_Port GPIOA
#define GREEN_Pin GPIO_PIN_10
#define GREEN_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SDCARD_DET_Pin GPIO_PIN_15
#define SDCARD_DET_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define READY_PWR_SENSE_Pin GPIO_PIN_5
#define READY_PWR_SENSE_GPIO_Port GPIOB
#define CHARGE_PWR_SENSE_Pin GPIO_PIN_7
#define CHARGE_PWR_SENSE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
