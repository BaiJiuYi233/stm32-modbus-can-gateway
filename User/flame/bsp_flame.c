/**
  ******************************************************************************
  * @file       bsp_flame.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      火焰传感器 GPIO 初始化
  *
  * @note       火焰模块的 DO 接 PA5，检测到火焰时输出低电平。
  *             具体有效电平以模块手册为准。
  ******************************************************************************
  */

#include "flame/bsp_flame.h"

/* 1 = 检测到火焰，0 = 正常；供 CAN 报警帧使用 */
volatile uint8_t flame_flags = 0;

/**
  * @brief  初始化火焰传感器 DO 引脚
  * @param  无
  * @retval 无
  * @note   模块 DO 是数字输出，这里按普通输入读取；
  *         如果模块是开漏输出，需要外部上拉或改用 GPIO_PULLUP。
  */
void flame_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. 使能 GPIOA 时钟（PA5 属于 GPIOA） */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 2. PA5 配置为数字输入 */
    GPIO_InitStruct.Pin = flame_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(flame_GPIO_Port, &GPIO_InitStruct);
}
