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
#include "oled.h"

        
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
    if(esp8266_cnt == 0)
        return REV_WAIT;

    if(esp8266_cnt == esp8266_cntPre)
    {
        // 接收数据稳定，返回 OK（不要在这里清空 esp8266_cnt，避免竞态）
        esp8266_cntPre = 0;
        return REV_OK;
    }

    esp8266_cntPre = esp8266_cnt;
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
    HAL_UART_Transmit(ESP8266_UART, (uint8_t *)cmd, strlen(cmd), 200);

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

    ESP8266_Clear();
    return false;
}

/**
*	函数名称：	ESP8266_Reset
*	函数功能：	ESP8266硬件复位
*/
//void ESP8266_Reset(void){

//    // 拉低RST引脚复位
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);
//    delay_ms(100);
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);
//    delay_ms(2000); // 等待模块重启
//}

/**
*	函数名称：	ESP8266_Init
*	函数功能：	ESP8266基础初始化（测试+模式+DHCP）
*/
void ESP8266_Init(void)
{

    //ESP8266_Reset(); // 硬件复位模块

    // 启动串口接收中断
    //HAL_UART_Receive_IT(ESP8266_UART, &esp8266_rx_byte, 1);
    // 启动串口 DMA 接收（替换原 HAL_UART_Receive_IT）
    HAL_UART_Receive_DMA(ESP8266_UART, (uint8_t*)esp8266_buf, ESP8266_BUF_SIZE - 1);
    
    // 测试AT指令
    if(ESP8266_SendCmd("AT\r\n", "OK"))
    {
    }
		else 
		{
				// 显示WiFi初始化失败
				OLED_Clear();
				OLED_ShowString(0, 0, (uint8_t*)"wifi init false", 8, 1);
				OLED_Refresh();
				return ;
    }
   HAL_UART_Transmit(ESP8266_UART, (uint8_t*)"AT test OK1\r\n", 11, 100);
    

    // 关闭回显，避免回显干扰后续匹配（ATE0）
    // 如果失败不致命，尝试多次但不死循环
    for (int i = 0; i < 3; ++i)
    {
        if (ESP8266_SendCmd("ATE0\r\n", "OK")) break;
        delay_ms(200);
    }


    // 设置Station模式
    if(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
    {
    }
		else{
				// 显示WiFi初始化失败
				OLED_Clear();
				OLED_ShowString(0, 0, (uint8_t*)"wifi init false", 8, 1);
				OLED_Refresh();
        return;
    }

    
    // 启用DHCP
    if(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
    {
    }
		else
		{
				// 显示WiFi初始化失败
				OLED_Clear();
				OLED_ShowString(0, 0, (uint8_t*)"wifi init false", 8, 1);
				OLED_Refresh();
				return ;
    }
}

/**
 *	函数名称：	ESP8266_ConnectWiFi
 *	函数功能：	连接WiFi网络
 *	返回参数：	true-连接成功	false-连接失败
 */
bool ESP8266_ConnectWiFi(void)
{
    //const char *wifi_cmd = "AT+CWJAP=\"mhuan\",\"12345678\"\r\n";
    char wifi_cmd[128];  // 注意预留足够空间
    snprintf(wifi_cmd, sizeof(wifi_cmd), 
             "AT+CWJAP=\"%s\",\"%s\"\r\n", 
             WIFI_SSID, WIFI_PASSWORD); 
    
    // 连接WiFi，等待GOT IP响应
    if(ESP8266_SendCmd(wifi_cmd, "GOT IP"))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 *	函数名称：	ESP8266_ConnectCloud
 *	函数功能：	连接OneNET云平台(MQTT)
 *	返回参数：	true-连接成功	false-连接失败
 */
bool ESP8266_ConnectCloud(void)
{
    // 先断开之前的MQTT连接（如果有）
    ESP8266_SendCmd("AT+MQTTDISCONN=0\r\n", "OK");
    delay_ms(500);
    
    // 清除之前的MQTT配置
    ESP8266_SendCmd("AT+MQTTUSERCFG=0,0,\"\",\"\",\"\",0,0,\"\"\r\n", "OK");
    delay_ms(500);
    
    char mqtt_user_cfg[512];
    snprintf(mqtt_user_cfg, sizeof(mqtt_user_cfg), 
             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
             DEVICE_NAME, PRODUCT_ID, MQTT_TOKEN);

    const char *mqtt_conn = "AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n";
    
    
    // 配置MQTT用户信息，最多尝试5次
    int mqtt_user_cfg_try = 0;
    bool mqtt_user_cfg_success = false;
    bool auto_connected = false;  // 标记是否自动连接
    
    while (mqtt_user_cfg_try < 10 && !mqtt_user_cfg_success)
    {
        HAL_UART_Transmit(ESP8266_UART, (uint8_t*)"Trying MQTT USERCFG...\r\n", 24, 100);
        
        // 清空接收缓冲区
        ESP8266_Clear();
        
        // 等待ESP8266空闲
        delay_ms(1000);
        
        // 发送长命令，增加超时时间到 5000ms
        HAL_UART_Transmit(ESP8266_UART, (uint8_t *)mqtt_user_cfg, strlen(mqtt_user_cfg), 5000);
        
        // 给ESP8266更多时间处理长命令
        delay_ms(1500);
        
        // 等待响应，增加超时时间
        uint16_t timeOut = 10000; // 增加到10秒
        bool got_response = false;
        
        while(timeOut--)
        {
            if(esp8266_cnt > 0) // 只要有数据就检查
            {
                // 等待更多字节到达
                delay_ms(300);
                
                char local_buf[ESP8266_BUF_SIZE];
                uint16_t len = esp8266_cnt;
                if (len > ESP8266_BUF_SIZE - 1) len = ESP8266_BUF_SIZE - 1;
                memcpy(local_buf, (const char*)esp8266_buf, len);
                if(strstr(local_buf, "OK") != NULL)
                {
                    got_response = true;
                    mqtt_user_cfg_success = true;
                    ESP8266_Clear();
                    break;
                }
                
                if(strstr(local_buf, "+MQTTCONNECTED") != NULL)
                {
                    got_response = true;
                    mqtt_user_cfg_success = true;
                    auto_connected = true;  // 标记为自动连接
                    ESP8266_Clear();
                    break;
                }
                
                if(strstr(local_buf, "ERROR") != NULL)
                {
                    got_response = true;
                    ESP8266_Clear();
                    break;
                }
                
                // 清空缓冲区，避免死循环打印
                ESP8266_Clear();
            }
            delay_ms(10);
        }
        
        if(mqtt_user_cfg_success)
        {
           
            if(auto_connected)
            {
               
            }
        }
        else
        {
            mqtt_user_cfg_try++;
            delay_ms(3000); // 等待3秒后重试
        }
    }
    
    if(!mqtt_user_cfg_success)
    {
        HAL_UART_Transmit(ESP8266_UART, (uint8_t*)"MQTT USERCFG failed after 10 attempts\r\n", 37, 100);
        return false;
    }
    
    // 如果没有自动连接，则手动发送连接命令
    if(!auto_connected)
    {
        HAL_UART_Transmit(ESP8266_UART, (uint8_t*)"Connecting MQTT...\r\n", 20, 100);
        
        // 连接MQTT服务器
        if(ESP8266_SendCmd(mqtt_conn, "+MQTTCONNECTED"))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    
    // 已经自动连接成功
    return true;
}

/**
 * 订阅 MQTT 主题
 * topic: 主题字符串，不带引号
 * qos: 0/1/2
 */
bool ESP8266_MQTT_Subscribe(const char *topic, uint8_t qos)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",%d\r\n", topic, qos);
    
    bool sub_result = ESP8266_SendCmd(cmd, "OK");
    
    if (sub_result)
    {
        return true;
    }
    else
    {
        delay_ms(200);
        
        sub_result = ESP8266_SendCmd(cmd, "OK");
        
        if (sub_result)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

/**
 * 发布 MQTT 消息
 * topic: 主题字符串，不带引号
 * payload: 要发送的 JSON 或文本
 * qos: 0/1/2
 * retain: 0/1
 */
/* 使用长度模式发布 MQTT（发送 AT+MQTTPUBRAW 带长度，等待 '>' 再发送 payload） */
bool ESP8266_MQTT_Publish(const char *topic, const char *payload, uint8_t qos, uint8_t retain)
{
    if(!topic || !payload) return false;

    char cmd[512];
    
    // 构造命令，使用与 tim.c 相同的格式，确保双引号和反斜杠正确转义
    // 格式：AT+MQTTPUB=0,"topic","payload",<qos>,<retain>
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",%d,%d\r\n", 
             topic, payload, qos, retain);


    // 清空接收缓存并发送命令
    ESP8266_Clear();
    HAL_UART_Transmit(ESP8266_UART, (uint8_t*)cmd, strlen(cmd), 4000);

    // 非阻塞式等待，使用短时间轮询，让出CPU时间给其他任务
    uint32_t start = HAL_GetTick();
    while((HAL_GetTick() - start) < 500)  // 最多等待500ms
    {
        // 检查是否收到 OK
        uint16_t len = esp8266_cnt;
        if (len)
        {
            if (len > ESP8266_BUF_SIZE - 1) len = ESP8266_BUF_SIZE - 1;
            char check_buf[ESP8266_BUF_SIZE];
            memcpy(check_buf, (const char*)esp8266_buf, len);
            check_buf[len] = '\0';
            if (strstr(check_buf, "OK") != NULL) 
            {
                return true;
            }
            if (strstr(check_buf, "ERROR") != NULL)
            {
                return false;
            }
        }
        // 短暂延时让出CPU，不阻塞其他任务
        delay_ms(1);
    }
    
    return false;
}

/**
 * 检查接收缓冲区，寻找订阅主题的下发消息并解析 JSON 控制 LED
 * 说明：主循环中定期调用该函数（避免在中断中做复杂解析）
 */
void ESP8266_ProcessMessages(void)
{
    char local_buf[ESP8266_BUF_SIZE];
    uint16_t len = 0;

    len = (esp8266_cnt < ESP8266_BUF_SIZE - 1) ? esp8266_cnt : (ESP8266_BUF_SIZE - 1);
    if (len == 0) return;  
    memcpy(local_buf, (const char*)esp8266_buf, len);
    // 清空计数以表示已消费（由主线程负责）
    esp8266_cnt = 0;
    ((char*)esp8266_buf)[0] = '\0';
    local_buf[len] = '\0';

    // 检查是否是MQTT订阅消息，如果不是则直接返回
    if (strstr(local_buf, "+MQTTSUBRECV") == NULL)
    {
       
        return;
    }

    // 找到第一个 JSON 并提取完整 JSON（简单配对）
    char *p_json = strchr(local_buf, '{');
    if (!p_json) 
    {
        
        return;
    }

    char json_buf[ESP8266_BUF_SIZE];
    char *p = p_json;
    int depth = 0;
    int idx = 0;
    while (*p && idx < (int)sizeof(json_buf) - 1)
    {
        if (*p == '{') depth++;
        if (*p == '}') depth--;
        json_buf[idx++] = *p++;
        if (depth == 0) break;
    }
    json_buf[idx] = '\0';
    if (idx == 0)
    {
        return;
    }

    
    // 检查 JSON 中是否包含 "LED":true
    if (strstr(json_buf, "\"LED\":true") != NULL)
    {
    }
    else if (strstr(json_buf, "\"LED\":false") != NULL)
    {
    }
    else
    {
    }

    // 构造并发送回执 payload
    char reply_payload[128];
    
    // 简单提取 id
    char *id_start = strstr(json_buf, "\"id\":\"");
    if (id_start)
    {
        id_start += 6; // 跳过 "id":" 
        char *id_end = strchr(id_start, '"');
        if (id_end)
        {
            *id_end = '\0';
            snprintf(reply_payload, sizeof(reply_payload),
                     "{\"id\":\"%s\",\"code\":200,\"data\":null,\"msg\":\"success\"}",
                     id_start);
            *id_end = '"'; // 恢复原始字符
        }
        else
        {
            snprintf(reply_payload, sizeof(reply_payload),
                     "{\"id\":\"%lu\",\"code\":200,\"data\":null,\"msg\":\"success\"}",
                     (unsigned long)HAL_GetTick());
        }
    }
    else
    {
        snprintf(reply_payload, sizeof(reply_payload),
                 "{\"id\":\"%lu\",\"code\":200,\"data\":null,\"msg\":\"success\"}",
                 (unsigned long)HAL_GetTick());
    }

    // 发布回执
    ESP8266_MQTT_Publish(MQTT_TOPIC_SET_REPLY, reply_payload, 0, 0);
}

