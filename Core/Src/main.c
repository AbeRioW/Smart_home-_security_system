/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht11.h"
#include "oled.h"
#include "usart.h"
#include "adc.h"
#include "stdio.h"
#include "stm32f1xx_hal_flash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
Thresholds thresholds = {
    .temperature = 30, // 默认温度阈值30
    .humidity = 20,    // 默认湿度阈值20
    .co2 = 2,          // 默认CO2阈值2
    .mq5 = 3000        // 默认MQ5阈值3000
};

SettingState setting_state = SETTING_NONE;
EditType current_edit = EDIT_TEMPERATURE;

// 按键标志位
uint8_t key1_pressed = 0;
uint8_t key2_pressed = 0;
uint8_t key3_pressed = 0;

// 显示更新标志位
uint8_t update_display = 0;

// Flash相关定义 - STM32F103C8T6 (64KB Flash)
#define ADDR_FLASH_PAGE_63      ((uint32_t)0x0800FC00)  // 第63页地址
#define FLASH_USER_START_ADDR   ADDR_FLASH_PAGE_63   // 选择最后一页
#define FLASH_USER_END_ADDR     ADDR_FLASH_PAGE_63 + FLASH_PAGE_SIZE - 1

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void FLASH_Write_Thresholds(Thresholds *th);
void FLASH_Read_Thresholds(Thresholds *th);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void Update_Setting_Display(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 写入阈值到Flash
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

// 从Flash读取阈值
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

// 按键中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 只设置标志位，不执行具体操作
    if (GPIO_Pin == KEY1_Pin) {
        key1_pressed = 1;
    } else if (GPIO_Pin == KEY2_Pin) {
        key2_pressed = 1;
    } else if (GPIO_Pin == KEY3_Pin) {
        key3_pressed = 1;
    }
}

// 更新设置界面显示
void Update_Setting_Display(void)
{
    if (setting_state == SETTING_NONE) {
        // 返回主界面，重新初始化显示
        OLED_Clear();
        DHT11_Data_TypeDef dht11_data;
        uint8_t result = DHT11_Read_Data(&dht11_data);
        if (result == 0) {
            OLED_ShowTempHumidity(&dht11_data);
        }
        OLED_ShowString(0, 8, (uint8_t*)"CO2: --.-- mg/m3", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"MQ5: ----/4095", 8, 1);
        OLED_Refresh();
    } else if (setting_state == SETTING_MODE) {
        // 设置界面，显示所有阈值
        static uint8_t last_edit = EDIT_TEMPERATURE;
        static Thresholds last_thresholds = {
            .temperature = 30,
            .humidity = 20,
            .co2 = 2,
            .mq5 = 3000
        };
        
        // 只有当编辑模式或阈值发生变化时才更新显示
        if (last_edit != current_edit || 
            last_thresholds.temperature != thresholds.temperature ||
            last_thresholds.humidity != thresholds.humidity ||
            last_thresholds.co2 != thresholds.co2 ||
            last_thresholds.mq5 != thresholds.mq5) {
            
            // 保存当前状态
            last_edit = current_edit;
            last_thresholds = thresholds;
            
            // 清空GRAM缓冲区
            uint8_t i, n;
            for (i = 0; i < 8; i++) {
                for (n = 0; n < 128; n++) {
                    OLED_GRAM[n][i] = 0;
                }
            }
            
            char buffer[32];
            
            // 显示温度阈值
            if (current_edit == EDIT_TEMPERATURE) {
                // 高亮显示当前编辑的阈值
                OLED_ShowString(0, 0, (uint8_t*)"Temp: ", 8, 1);
                sprintf(buffer, "%d C", thresholds.temperature);
                OLED_ShowString(48, 0, (uint8_t*)buffer, 8, 0); // 反显
            } else {
                sprintf(buffer, "Temp: %d C", thresholds.temperature);
                OLED_ShowString(0, 0, (uint8_t*)buffer, 8, 1);
            }
            
            // 显示湿度阈值
            if (current_edit == EDIT_HUMIDITY) {
                // 高亮显示当前编辑的阈值
                OLED_ShowString(0, 10, (uint8_t*)"Humidity: ", 8, 1);
                sprintf(buffer, "%d %%", thresholds.humidity);
                OLED_ShowString(64, 10, (uint8_t*)buffer, 8, 0); // 反显
            } else {
                sprintf(buffer, "Humidity: %d %%", thresholds.humidity);
                OLED_ShowString(0, 10, (uint8_t*)buffer, 8, 1);
            }
            
            // 显示CO2阈值
            if (current_edit == EDIT_CO2) {
                // 高亮显示当前编辑的阈值
                OLED_ShowString(0, 20, (uint8_t*)"CO2: ", 8, 1);
                sprintf(buffer, "%d ppm", thresholds.co2);
                OLED_ShowString(48, 20, (uint8_t*)buffer, 8, 0); // 反显
            } else {
                sprintf(buffer, "CO2: %d ppm", thresholds.co2);
                OLED_ShowString(0, 20, (uint8_t*)buffer, 8, 1);
            }
            
            // 显示MQ5阈值
            if (current_edit == EDIT_MQ5) {
                // 高亮显示当前编辑的阈值
                OLED_ShowString(0, 30, (uint8_t*)"MQ5: ", 8, 1);
                sprintf(buffer, "%d", thresholds.mq5);
                OLED_ShowString(48, 30, (uint8_t*)buffer, 8, 0); // 反显
            } else {
                sprintf(buffer, "MQ5: %d", thresholds.mq5);
                OLED_ShowString(0, 30, (uint8_t*)buffer, 8, 1);
            }
            
            // 显示操作提示
            OLED_ShowString(0, 40, (uint8_t*)"KEY1: Next", 8, 1);
            OLED_ShowString(0, 50, (uint8_t*)"KEY2: +  KEY3: -", 8, 1);
            
            // 刷新显示
            OLED_Refresh();
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  UART_Start_Receive();
  
  // 从Flash读取阈值
  FLASH_Read_Thresholds(&thresholds);
  
  DHT11_Data_TypeDef dht11_data;
  uint8_t result;
  uint32_t dht11_timer = 0;
  uint32_t uart_error_count = 0;
  
  result = DHT11_Read_Data(&dht11_data);
  if (result == 0)
  {
    OLED_ShowTempHumidity(&dht11_data);
  }
  else
  {
    char error_str[20];
    sprintf(error_str, "DHT11 Err:%d", result);
    OLED_ShowString(0, 0, (uint8_t*)error_str, 8, 1);
    OLED_Refresh();
  }
  
  OLED_ShowString(0, 8, (uint8_t*)"CO2: --.-- mg/m3", 8, 1);
  OLED_Refresh_Line(1);
  
  OLED_ShowString(0, 16, (uint8_t*)"MQ5: ----/4095", 8, 1);
  OLED_Refresh_Line(2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 处理按键事件
    if (key1_pressed) {
        key1_pressed = 0;
        // KEY1按下，切换设置状态或当前编辑的阈值类型
        if (setting_state == SETTING_NONE) {
            setting_state = SETTING_MODE;
            current_edit = EDIT_TEMPERATURE;
        } else if (setting_state == SETTING_MODE) {
            // 切换当前编辑的阈值类型
            if (current_edit == EDIT_TEMPERATURE) {
                current_edit = EDIT_HUMIDITY;
            } else if (current_edit == EDIT_HUMIDITY) {
                current_edit = EDIT_CO2;
            } else if (current_edit == EDIT_CO2) {
                current_edit = EDIT_MQ5;
            } else if (current_edit == EDIT_MQ5) {
                setting_state = SETTING_NONE;
                // 保存阈值到Flash
                FLASH_Write_Thresholds(&thresholds);
            }
        }
        update_display = 1;
    }
    
    if (key2_pressed) {
        key2_pressed = 0;
        // KEY2按下，增加当前编辑的阈值
        if (setting_state == SETTING_MODE) {
            switch (current_edit) {
                case EDIT_TEMPERATURE:
                    thresholds.temperature++;
                    if (thresholds.temperature > 40) {
                        thresholds.temperature = 10;
                    }
                    break;
                case EDIT_HUMIDITY:
                    thresholds.humidity++;
                    if (thresholds.humidity > 40) {
                        thresholds.humidity = 10;
                    }
                    break;
                case EDIT_CO2:
                    thresholds.co2++;
                    if (thresholds.co2 > 10) {
                        thresholds.co2 = 1;
                    }
                    break;
                case EDIT_MQ5:
                    thresholds.mq5 += 100;
                    if (thresholds.mq5 > 4000) {
                        thresholds.mq5 = 2000;
                    }
                    break;
            }
            update_display = 1;
        }
    }
    
    if (key3_pressed) {
        key3_pressed = 0;
        // KEY3按下，减少当前编辑的阈值
        if (setting_state == SETTING_MODE) {
            switch (current_edit) {
                case EDIT_TEMPERATURE:
                    thresholds.temperature--;
                    if (thresholds.temperature < 10) {
                        thresholds.temperature = 40;
                    }
                    break;
                case EDIT_HUMIDITY:
                    thresholds.humidity--;
                    if (thresholds.humidity < 10) {
                        thresholds.humidity = 40;
                    }
                    break;
                case EDIT_CO2:
                    thresholds.co2--;
                    if (thresholds.co2 < 1) {
                        thresholds.co2 = 10;
                    }
                    break;
                case EDIT_MQ5:
                    thresholds.mq5 -= 100;
                    if (thresholds.mq5 < 2000) {
                        thresholds.mq5 = 4000;
                    }
                    break;
            }
            update_display = 1;
        }
    }
    
    // 更新显示
    if (update_display) {
        update_display = 0;
        Update_Setting_Display();
    }
    
    if (setting_state == SETTING_NONE)
    {
      dht11_timer++;
      
      if (dht11_timer >= 50)
      {
        dht11_timer = 0;
        result = DHT11_Read_Data(&dht11_data);
        if (result == 0)
        {
          OLED_ShowTempHumidity(&dht11_data);
        }
        else
        {
          // 读取失败，保持之前的显示
        }
      }
      
      if (uart_data.data_ready)
      {
        uart_data.data_ready = 0;
        
        if (UART_Parse_Data() == 0)
        {
          float co2 = UART_Get_CO2();
          
          OLED_ShowGasConcentration(0, 0, co2);
        }
        else
        {
          uart_error_count++;
        }
      }
      
      // 读取并显示MQ5数值
      uint16_t mq5_value = MQ5_Read_Value();
      OLED_ShowMQ5Value(mq5_value);
    }
    
    HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
