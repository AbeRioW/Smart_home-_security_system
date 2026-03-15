/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : flash.h
  * @brief          : Flash操作相关头文件
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLASH_H
#define __FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
// 阈值结构体
typedef struct {
    uint8_t temperature; // 温度阈值，范围10-40
    uint8_t humidity;    // 湿度阈值，范围10-90
    float co2;           // CO2阈值，单位mg/m3，范围0.1-10.0
    uint16_t mq5;        // MQ5阈值，范围2000-5000
} Thresholds;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
// Flash相关定义 - STM32F103C8T6 (64KB Flash)
#define ADDR_FLASH_PAGE_63      ((uint32_t)0x0800FC00)  // 第63页地址
#define FLASH_USER_START_ADDR   ADDR_FLASH_PAGE_63   // 选择最后一页
#define FLASH_USER_END_ADDR     ADDR_FLASH_PAGE_63 + FLASH_PAGE_SIZE - 1

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */
void FLASH_Write_Thresholds(Thresholds *th);
void FLASH_Read_Thresholds(Thresholds *th);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_H */

