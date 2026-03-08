/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
UART_Data_TypeDef uart_data = {0};

void UART_Start_Receive(void)
{
    uart_data.rx_index = 0;
    uart_data.data_ready = 0;
    HAL_UART_Receive_IT(&huart1, &uart_data.rx_buffer[0], 1);
}

uint8_t UART_Parse_Data(void)
{
    uint8_t i;
    uint16_t checksum = 0;
    
    if (uart_data.rx_buffer[0] != 0x2C || uart_data.rx_buffer[1] != 0xE4)
    {
        return 1;
    }
    
    for (i = 0; i < 8; i++)
    {
        checksum += uart_data.rx_buffer[i];
    }
    
    if ((checksum & 0xFF) != uart_data.rx_buffer[8])
    {
        return 2;
    }
    
    uart_data.tvoc_value = (uart_data.rx_buffer[2] << 8) | uart_data.rx_buffer[3];
    uart_data.ch2o_value = (uart_data.rx_buffer[4] << 8) | uart_data.rx_buffer[5];
    uart_data.co2_value = (uart_data.rx_buffer[6] << 8) | uart_data.rx_buffer[7];
    
    return 0;
}

float UART_Get_TVOC(void)
{
    return uart_data.tvoc_value * 0.001f;
}

float UART_Get_CH2O(void)
{
    return uart_data.ch2o_value * 0.001f;
}

float UART_Get_CO2(void)
{
    return uart_data.co2_value * 0.001f;
}

void UART_Show_Received_Data(void)
{
    uint8_t i;
    OLED_ShowString(0, 8, (uint8_t*)"RX:", 8, 1);
    
    for (i = 0; i < 9; i++)
    {
        OLED_ShowNum(24 + i * 12, 8, uart_data.rx_buffer[i], 2, 8, 1);
        if (i < 8)
        {
            OLED_ShowChar(36 + i * 12, 8, ' ', 8, 1);
        }
    }
    
    OLED_Refresh_Line(1);
}
/* USER CODE END 1 */

