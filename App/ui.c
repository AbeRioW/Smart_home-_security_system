#include "ui.h"

// 变量定义
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

// 其他变量
DHT11_Data_t dht11_data;
uint8_t result;
uint32_t dht11_timer = 0;
uint32_t uart_error_count = 0;

/**
 * @brief 初始化UI
 * @param 无
 * @retval 无
 */
void UI_Init(void)
{
    OLED_Init();
   // FLASH_Read_Thresholds(&thresholds);
    
    //result = DHT11_READ_DATA(&dht11_data);
    if (result == 0)
    {
        OLED_ShowTempHumidity(&dht11_data);
    }
    else
    {
        char error_str[20];
        sprintf(error_str, "DHT11 Err: Read Failed");
        OLED_ShowString(0, 0, (uint8_t*)error_str, 8, 1);
        OLED_Refresh();
    }
    
   // OLED_ShowString(0, 8, (uint8_t*)"CO2: --.-- mg/m3", 8, 1);
  //  OLED_Refresh_Line(1);
    
    //OLED_ShowString(0, 16, (uint8_t*)"MQ5: ----/4095", 8, 1);
   // OLED_Refresh_Line(2);
}

/**
 * @brief 更新UI
 * @param 无
 * @retval 无
 */
void UI_Update(void)
{
    UI_Handle_Key_Events();
    
    // 更新显示
    if (update_display) {
        update_display = 0;
        Update_Setting_Display();
    }
    
    if (setting_state == SETTING_NONE)
    {
        UI_Update_Main_Display();
    }
}

/**
 * @brief 处理按键事件
 * @param 无
 * @retval 无
 */
void UI_Handle_Key_Events(void)
{
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
}

/**
 * @brief 更新主界面显示
 * @param 无
 * @retval 无
 */
void UI_Update_Main_Display(void)
{
    dht11_timer++;
    
    if (dht11_timer >= 50)
    {
        dht11_timer = 0;
        result = DHT11_READ_DATA(&dht11_data);
        if (result == 0)
        {
            OLED_ShowTempHumidity(&dht11_data);
        }
        else
        {
            // 读取失败，显示错误信息
            char error_str[30];
            sprintf(error_str, "DHT11 Err: Read Failed");
            // 先清空第一行
            uint8_t i;
            for (i = 0; i < 128; i++)
            {
                OLED_GRAM[i][0] = 0;
            }
            OLED_ShowString(0, 0, (uint8_t*)error_str, 8, 1);
            OLED_Refresh_Line(0);
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

/**
 * @brief 更新设置界面显示
 * @param 无
 * @retval 无
 */
void Update_Setting_Display(void)
{
    if (setting_state == SETTING_NONE) {
        // 返回主界面，重新初始化显示
        OLED_Clear();
        DHT11_Data_t dht11_data;
        uint8_t result = DHT11_READ_DATA(&dht11_data);
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

/**
 * @brief 按键中断回调函数
 * @param GPIO_Pin: 中断的GPIO引脚
 * @retval 无
 */
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
