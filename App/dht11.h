#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"
#include "gpio.h"
#include "stdio.h"
#include "stdbool.h"


#define DHT11_PIN_SET   HAL_GPIO_WritePin(DHT11_GPIO_Port,DHT11_Pin,GPIO_PIN_SET)                                   //  ����GPIOΪ��
#define DHT11_PIN_RESET HAL_GPIO_WritePin(DHT11_GPIO_Port,DHT11_Pin,GPIO_PIN_RESET)                                 //  ����GPIOΪ��
#define DHT11_READ_IO   HAL_GPIO_ReadPin(DHT11_GPIO_Port,DHT11_Pin)                                                 //  DHT11 GPIO����

typedef struct {
    uint8_t humidity_int;    // ʪ����������
    uint8_t humidity_dec;    // ʪ��С�����֣�DHT11ʼ��Ϊ0��
    uint8_t temp_int;        // �¶���������
    uint8_t temp_dec;        // �¶�С�����֣�DHT11ʼ��Ϊ0��
    uint8_t checksum;        // У���
} DHT11_Data_t;

void DHT11_START(void);
unsigned char DHT11_READ_BIT(void);
unsigned char DHT11_READ_BYTE(void);
unsigned char DHT11_READ_DATA(DHT11_Data_t *dht_data);
unsigned char DHT11_Check(void);
static void DHT11_GPIO_MODE_SET(uint8_t mode);

void Coarse_delay_us(uint32_t us);

#endif