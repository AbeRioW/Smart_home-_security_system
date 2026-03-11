/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : flash.c
  * @brief          : Flash操作相关实现
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

/* Includes ------------------------------------------------------------------*/
#include "flash.h"
#include "stm32f1xx_hal_flash.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  写入阈值到Flash
  * @param  th: 阈值结构体指针
  * @retval None
  */
void FLASH_Write_Thresholds(Thresholds *th)
{
    HAL_FLASH_Unlock();
    
    // 擦除Flash页
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
    EraseInitStruct.NbPages = 1;
    
    uint32_t PageError = 0;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    // 写入阈值数据
    uint32_t *data_ptr = (uint32_t*)th;
    for (int i = 0; i < sizeof(Thresholds)/4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_START_ADDR + i*4, data_ptr[i]);
    }
    
    HAL_FLASH_Lock();
}

/**
  * @brief  从Flash读取阈值
  * @param  th: 阈值结构体指针
  * @retval None
  */
void FLASH_Read_Thresholds(Thresholds *th)
{
    uint32_t *data_ptr = (uint32_t*)th;
    for (int i = 0; i < sizeof(Thresholds)/4; i++) {
        data_ptr[i] = *(__IO uint32_t*)(FLASH_USER_START_ADDR + i*4);
    }
    
    // 检查数据有效性，如果无效则使用默认值
    if (th->temperature < 10 || th->temperature > 40 ||
        th->humidity < 10 || th->humidity > 40 ||
        th->co2 < 1 || th->co2 > 10 ||
        th->mq5 < 2000 || th->mq5 > 4000) {
        // 数据无效，使用默认值
        th->temperature = 30;
        th->humidity = 20;
        th->co2 = 2;
        th->mq5 = 3000;
    }
}

/* USER CODE END 0 */