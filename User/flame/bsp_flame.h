#ifndef __BSP_FLAME_H
#define __BSP_FLAME_H

#include "main.h"

/* 火焰传感器 DO 接 PA5，检测到火焰时通常为低电平 */
#define flame_Pin          GPIO_PIN_5
#define flame_GPIO_Port    GPIOA

/* 读取火焰 DO：返回 GPIO_PIN_SET 或 GPIO_PIN_RESET */
#define flame_Read()         HAL_GPIO_ReadPin(flame_GPIO_Port, flame_Pin)

/* 1 = 检测到火焰，0 = 正常 */
extern volatile uint8_t flame_flags;

/* 初始化火焰 DO 引脚 */
void flame_init(void);

#endif
