#include "delay.h"

static uint32_t fac_us = 0;  // 微秒延时系数
static uint32_t fac_ms = 0;  // 毫秒延时系数

// 初始化延时函数（外部晶振专用）
void Delay_Init(void) {
    // 外部晶振配置下，SystemCoreClock为实际系统时钟频率
    // 例如：8MHz外部晶振经PLL倍频到72MHz，则SystemCoreClock=72000000
    fac_us = SystemCoreClock / 1000000;  // 计算1us需要的计数次数
    fac_ms = fac_us * 1000;              // 计算1ms需要的计数次数
}

// 微秒级延时
void delay_us(uint32_t us) {
    uint32_t temp;
    SysTick->LOAD = us * fac_us;  // 加载计数值
    SysTick->VAL = 0x00;          // 清空计数器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // 启动计数器

    // 等待计数结束
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;  // 关闭计数器
    SysTick->VAL = 0x00;                        // 清空计数器
}

// 毫秒级延时
void delay_ms(uint32_t ms) {
    uint32_t i;
    for (i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

