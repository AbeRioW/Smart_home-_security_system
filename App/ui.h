#ifndef UI_H
#define UI_H

#include "flash.h"
#include "dht11.h"
#include "oled.h"
#include "usart.h"
#include "adc.h"
#include <stdio.h>

// 设置状态枚举
typedef enum {
    SETTING_NONE,
    SETTING_MODE
} SettingState;

// 编辑类型枚举
typedef enum {
    EDIT_TEMPERATURE,
    EDIT_HUMIDITY,
    EDIT_CO2,
    EDIT_MQ5
} EditType;

// 外部变量声明
extern Thresholds thresholds;
extern SettingState setting_state;
extern EditType current_edit;
extern uint8_t key1_pressed;
extern uint8_t key2_pressed;
extern uint8_t key3_pressed;
extern uint8_t update_display;

// 函数原型
extern void UI_Init(void);
extern void UI_Update(void);
extern void UI_Handle_Key_Events(void);
extern void UI_Update_Main_Display(void);
extern void Update_Setting_Display(void);
extern void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* UI_H */
