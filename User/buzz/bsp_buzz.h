#ifndef __BSP_BUZZ_H
#define __BSP_BUZZ_H


#include "main.h"

/* ------------------------- 初始化引脚 ------------------------- */
#define buzz_Pin        GPIO_PIN_4
#define buzz_GPIO_Port  GPIOA

/* ------------------------- 蜂鸣器的启动和关闭 ------------------------- */
#define BUZZ_ON()         HAL_GPIO_WritePin(buzz_GPIO_Port , buzz_Pin , GPIO_PIN_SET);
#define BUZZ_OFF()        HAL_GPIO_WritePin(buzz_GPIO_Port , buzz_Pin , GPIO_PIN_RESET);

/* ------------------------- 函数声明 ------------------------- */
void Buzz_Init(void);//蜂鸣器初始化
	
#endif