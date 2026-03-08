#include "dht11.h"

static void DHT11_Delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    uint32_t start = SysTick->VAL;
    while ((start - SysTick->VAL) < ticks);
}

void DHT11_GPIO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

void DHT11_GPIO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

uint8_t DHT11_Read_Data(DHT11_Data_TypeDef *data)
{
    uint8_t i, j;
    uint8_t temp = 0;
    uint8_t buffer[5] = {0};
    uint32_t timeout;
    
    // 发送开始信号
    DHT11_GPIO_Output();
    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);  // 拉低至少18ms
    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);
    DHT11_Delay_us(30);  // 拉高30us
    
    // 切换到输入模式
    DHT11_GPIO_Input();
    
    // 等待DHT11响应
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET)
    {
        if (--timeout == 0) return 1;  // 未收到响应
    }
    
    // 等待响应低电平结束
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_RESET)
    {
        if (--timeout == 0) return 2;  // 响应低电平超时
    }
    
    // 等待响应高电平结束
    timeout = 10000;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET)
    {
        if (--timeout == 0) return 3;  // 响应高电平超时
    }
    
    // 读取40位数据
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            // 等待数据位开始
            timeout = 10000;
            while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_RESET)
            {
                if (--timeout == 0) return 4;  // 数据位开始超时
            }
            
            // 测量高电平持续时间
            uint32_t high_time = 0;
            while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET)
            {
                high_time++;
                if (high_time > 1000) break;  // 防止无限循环
            }
            
            // 根据高电平持续时间判断数据位
            if (high_time > 30)  // 高电平持续时间大于30us为1位
            {
                temp |= (uint8_t)(0x01 << (7 - j));
            }
        }
        buffer[i] = temp;
        temp = 0;
    }
    
    // 校验数据
    if ((buffer[0] + buffer[1] + buffer[2] + buffer[3]) != buffer[4])
    {
        return 6;  // 校验错误
    }
    
    // 填充数据
    data->humidity_int = buffer[0];
    data->humidity_dec = buffer[1];
    data->temperature_int = buffer[2];
    data->temperature_dec = buffer[3];
    data->check_sum = buffer[4];
    
    return 0;
}