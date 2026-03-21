#include "esp8266.h"
#include "stm32f103xb.h"
#include "usart.h"
#include "delay.h"
#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include <stdarg.h>
#include "cJSON.h"
#include "gpio.h"
#include "stdlib.h"

        
// 全局变量定义
volatile uint8_t esp8266_buf[ESP8266_BUF_SIZE];          //接收缓冲区，存储 ESP8266 返回的数据
volatile uint16_t esp8266_cnt = 0, esp8266_cntPre = 0;   //缓冲区计数变量，用于判断接收是否完成
// uint8_t esp8266_rx_byte = 0;                    //中断接收的单个字节临时存储(可保留或删除，DMA 不用)

bool led1_status = 0; // LED1 状态变量
bool led2_status = 0; // LED2 状态变量

/* 等待缓冲中出现指定字符串（简单实现，单位 ms） */
static bool ESP8266_WaitForStr(const char *str, uint32_t timeout_ms)
{
    uint32_t ticks = timeout_ms / 10;
    while(ticks--)
    {
        delay_ms(10);
     // 快照长度并复制到本地缓冲，避免并发读取问题
        uint16_t len = esp8266_cnt;
        if (len)
        {
            if (len > ESP8266_BUF_SIZE - 1) len = ESP8266_BUF_SIZE - 1;
            char check_buf[ESP8266_BUF_SIZE];
            memcpy(check_buf, (const char*)esp8266_buf, len);
            check_buf[len] = '\0';
            if (str && strstr(check_buf, str) != NULL) return true;
        }
    }
    return false;
}

/**
*	函数名称：	ESP8266_Clear
*	函数功能：	清空接收缓存
*/
void ESP8266_Clear(void)
{
    memset((void *)esp8266_buf, 0, sizeof(esp8266_buf));
    esp8266_cnt = 0;
}

/**
*   函数名称：	ESP8266_WaitRecive
*	函数功能：	等待接收完成
*	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
*/
bool ESP8266_WaitRecive(void)
{
    static uint32_t last_time = 0;
    
    if(esp8266_cnt == 0)
    {
        last_time = 0;
        return REV_WAIT;
    }

    // 第一次接收到数据，记录时间
    if (last_time == 0)
    {
        last_time = HAL_GetTick();
        esp8266_cntPre = esp8266_cnt;
        return REV_WAIT;
    }

    // 检查数据是否稳定（100ms内没有变化）
    if(esp8266_cnt == esp8266_cntPre)
    {
        if (HAL_GetTick() - last_time > 100)
        {
            // 接收数据稳定，返回 OK（不要在这里清空 esp8266_cnt，避免竞态）
            esp8266_cntPre = 0;
            last_time = 0;
            return REV_OK;
        }
    }
    else
    {
        // 数据有变化，更新时间和计数
        last_time = HAL_GetTick();
        esp8266_cntPre = esp8266_cnt;
    }

    return REV_WAIT;
}

/**
*	函数名称：	ESP8266_SendCmd
*	函数功能：	发送AT指令并等待响应
*	入口参数：	cmd：指令内容	expected_resp：期望的响应
*	返回参数：	true-成功	false-失败
*/
bool ESP8266_SendCmd(const char *cmd, const char *expected_resp)
{
  uint16_t timeOut = 3000; // 原 200 -> 3000 (约30s)

    // 清空接收缓冲区
    ESP8266_Clear();

    // 发送指令
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 3000);

    // 给模块一点时间开始回复（针对长命令适当延时）
    delay_ms(50);

    // 等待响应
    while(timeOut--)
    {
        if(ESP8266_WaitRecive() == REV_OK)
        {
            // 等待更多字节到达，避免分片丢失（可根据需要增大）
            delay_ms(200);

            // 拷贝缓冲到本地（用 esp8266_cnt 快照）
            char local_buf[ESP8266_BUF_SIZE];
            uint16_t len = esp8266_cnt;
            if (len > ESP8266_BUF_SIZE - 1) len = ESP8266_BUF_SIZE - 1;
            memcpy(local_buf, (const char*)esp8266_buf, len);
            local_buf[len] = '\0';

            // 检查local_buf是否为空或只包含空白字符，如果是，继续等待
            bool has_data = false;
            for (uint16_t i = 0; i < len; i++)
            {
                if (local_buf[i] != ' ' && local_buf[i] != '\t' && local_buf[i] != '\r' && local_buf[i] != '\n')
                {
                    has_data = true;
                    break;
                }
            }

            if (!has_data)
            {
                ESP8266_Clear();
                continue;
            }

            // 调试输出完整接收内容
            printf("--ESP RX: %s\r\n", local_buf);

            // 如果包含期望响应，返回成功
            if(expected_resp && strstr(local_buf, expected_resp) != NULL)
            {
                ESP8266_Clear();
                return true;
            }

            // 如果包含 ERROR，立即返回失败（并打印）
            if(strstr(local_buf, "ERROR") != NULL)
            {
                ESP8266_Clear();
                return false;
            }

            // 否则继续等待（可能是回显或其它异步信息）
        }
        delay_ms(10);
    }

      // 超时，拷贝并打印一次缓冲以便排查
    {
        char dump_buf[ESP8266_BUF_SIZE];
        uint16_t dump_len = esp8266_cnt;
        if (dump_len > ESP8266_BUF_SIZE - 1) dump_len = ESP8266_BUF_SIZE - 1;
        memcpy(dump_buf, (const char*)esp8266_buf, dump_len);
        dump_buf[dump_len] = '\0';

        printf("--ESP RX on timeout: %s\r\n", dump_buf);
    }

    ESP8266_Clear();
    return false;
}

/**
*	函数名称：	ESP8266_Init
*	函数功能：	ESP8266基础初始化（测试+模式+DHCP）
*/
void ESP8266_Init(void)
{
    // 启动串口 DMA 接收（替换原 HAL_UART_Receive_IT）
    HAL_UART_Receive_DMA(&huart2, (uint8_t*)esp8266_buf, ESP8266_BUF_SIZE - 1);
    			  	#if DEBUG
						printf("AT check\r\n");
					#endif
    // 测试AT指令
    if(ESP8266_SendCmd("AT\r\n", "OK"))
    {
        delay_ms(500);
			  	#if DEBUG
						printf("AT check ok\r\n");
					#endif
        //ESP8266_SendCmd("AT+GMR\r\n", "OK");
        delay_ms(500);
    }else {
				#if DEBUG
					printf("AT test failed, retrying...\r\n");
				#endif
        delay_ms(500);
    }
    #if DEBUG
			printf("AT test OK1\r\n");
		#endif

    // 关闭回显，避免回显干扰后续匹配（ATE0）
    // 如果失败不致命，尝试多次但不死循环
    for (int i = 0; i < 3; ++i)
    {
			    #if DEBUG
			printf("ATE0\r\n");
		#endif
        if (ESP8266_SendCmd("ATE0\r\n", "OK")) break;
        delay_ms(200);
    }


    // 设置Station模式
    if(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
    {    
			#if DEBUG
			printf("CWMODE set to 1 OK\r\n");
		#endif
        delay_ms(500);
    }else{
			#if DEBUG
				printf("Set CWMODE failed, retrying...\r\n");
			#endif
        delay_ms(500);
    }

    
    // 启用DHCP
    if(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
    {
			#if DEBUG
				printf("DHCP enabled OK\r\n");
			#endif
        delay_ms(500);
    }else{
			#if DEBUG
				printf("Set DHCP failed, retrying...\r\n");
			#endif
        delay_ms(500);
    }
}

