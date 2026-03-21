/*
 * delay.h
 *
 *  Created on: Sep 1, 2025
 *      Author: mhuan 
 */

#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#include "stm32f1xx_hal.h"

void Delay_Init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif /* INC_DELAY_H_ */
