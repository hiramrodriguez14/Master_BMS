#ifndef TEST_ANALOG_HAL_H
#define TEST_ANALOG_HAL_H
#define GPIO_PIN_1 2
#define GPIO_PIN_2 4
#define GPIO_MODE_ANALOG 3
#define OPAMP1 1
#define ADC1 1
#define OPAMP_FOLLOWER_MODE 1
#define OPAMP_NONINVERTINGINPUT_IO0 1
#define OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE 0
#define OPAMP_TRIMMING_FACTORY 0
#define ADC_CLOCK_SYNC_PCLK_DIV4 4
#define ADC_RESOLUTION_12B 12
#define ADC_DATAALIGN_RIGHT 0
#define ADC_SCAN_DISABLE 0
#define ADC_EOC_SINGLE_CONV 0
#define ADC_SOFTWARE_START 0
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0
#define ADC_OVR_DATA_PRESERVED 0
#define ADC_CHANNEL_3 3
#define ADC_REGULAR_RANK_1 1
#define ADC_SAMPLETIME_181CYCLES_5 181
#define ADC_SINGLE_ENDED 0
#define ADC_OFFSET_NONE 0
#define DISABLE 0
#define __HAL_RCC_GPIOA_CLK_ENABLE() ((void)0)
#define __HAL_RCC_SYSCFG_CLK_ENABLE() ((void)0)
#define __HAL_RCC_ADC12_CLK_ENABLE() ((void)0)
typedef struct {int Instance; struct {int Mode, NonInvertingInput, TimerControlledMuxmode, UserTrimming;} Init;} OPAMP_HandleTypeDef;
typedef struct {int Instance; struct {int ClockPrescaler,Resolution,DataAlign,ScanConvMode,EOCSelection,LowPowerAutoWait,ContinuousConvMode,NbrOfConversion,DiscontinuousConvMode,ExternalTrigConv,ExternalTrigConvEdge,DMAContinuousRequests,Overrun;} Init;} ADC_HandleTypeDef;
typedef struct {int Channel,Rank,SamplingTime,SingleDiff,OffsetNumber;} ADC_ChannelConfTypeDef;
HAL_StatusTypeDef HAL_OPAMP_Init(OPAMP_HandleTypeDef *);
HAL_StatusTypeDef HAL_OPAMP_SelfCalibrate(OPAMP_HandleTypeDef *);
HAL_StatusTypeDef HAL_OPAMP_Start(OPAMP_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *,uint32_t);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *,uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *);
#endif
