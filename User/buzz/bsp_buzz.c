/**
  ******************************************************************************
  * @file       bsp_buzz.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      蜂鸣器模块
  *
  ******************************************************************************
	*/
#include "buzz/bsp_buzz.h"

void Buzz_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //启动GPIOA内部时钟
  __HAL_RCC_GPIOA_CLK_ENABLE();

  //初始写入低电平
  HAL_GPIO_WritePin(buzz_GPIO_Port, buzz_Pin, GPIO_PIN_RESET);

  
  GPIO_InitStruct.Pin = buzz_Pin; 						//蜂鸣器引脚
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	//GPIO输出模式
  GPIO_InitStruct.Pull = GPIO_NOPULL;					//不上拉不下拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;//低速输出
  HAL_GPIO_Init(buzz_GPIO_Port, &GPIO_InitStruct);

}