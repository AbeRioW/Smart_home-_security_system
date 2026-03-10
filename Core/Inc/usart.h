/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
#define UART_RX_BUF_SIZE 20
#define UART_DATA_LENGTH 9
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void UART_Start_Receive(void);
uint8_t UART_Parse_Data(void);
float UART_Get_TVOC(void);
float UART_Get_CH2O(void);
float UART_Get_CO2(void);
void UART_Show_Received_Data(void);
/* USER CODE END Prototypes */

/* USER CODE BEGIN Private variables */
extern UART_Data_TypeDef uart_data;
/* USER CODE END Private variables */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

