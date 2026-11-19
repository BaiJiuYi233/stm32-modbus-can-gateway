/**
  ******************************************************************************
  * @file       board_select.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      A/B/C 板级初始化
  *
  * @note       根据 board_select.h 里的 BOARD_HAS_* / BOARD_MODBUS_* 宏，
  *             只初始化当前板型需要的外设。
  ******************************************************************************
  */

#include "board/board_select.h"
#include "led/bsp_led.h"
#include "dwt/bsp_dwt.h"
#include "usart/bsp_usart.h"
#include "adc/bsp_adc.h"

/* 只有带 CAN 的板子才包含 CAN BSP */
#if BOARD_HAS_CAN
#include "can/bsp_can.h"
#endif

/* 只有 C 板才有蜂鸣器 */
#if BOARD_HAS_BUZZ
#include "buzz/bsp_buzz.h"
#endif

/* 只有 C 板才有火焰传感器 */
#if BOARD_HAS_FLAME
#include "flame/bsp_flame.h"
#endif

/* 只有 A/B 板才有 RS485 */
#if BOARD_HAS_RS485
#include "rs485/bsp_rs485.h"
#endif

/* 只有 B 板是从站，需要创建 Modbus 队列 */
#if BOARD_MODBUS_SLAVE
#include "modbus/bsp_modbus.h"
#endif

/**
  * @brief  初始化当前板型的基础外设
  * @param  无
  * @retval 无
  * @note   顺序很重要：
  *         1. 先初始化 LED/调试串口/DWT；
  *         2. 再初始化 CAN、蜂鸣器、火焰；
  *         3. B 板先建 Modbus 队列，再开 RS485 接收；
  *         4. 最后初始化 ADC。
  */
void Board_Init(void)
{
    /* 基础 BSP：LED、调试串口、微秒延时 */
    LED_GPIO_Init();
    Debug_UART_Init();
    DWT_Init();

    /* A/C 板初始化 CAN；B 板没有 CAN */
#if BOARD_HAS_CAN
    CAN_Init();
#endif

    /* C 板初始化蜂鸣器 */
#if BOARD_HAS_BUZZ
    Buzz_Init();
#endif

    /* C 板初始化火焰传感器 DO */
#if BOARD_HAS_FLAME
    flame_init();
#endif

    /* B 板：先创建 Modbus 队列，再打开 RS485 接收中断。
       顺序反了可能在中断里拿到空队列。 */
#if BOARD_MODBUS_SLAVE
    Modbus_Slave_Init();
#endif

    /* A/B 板初始化 RS485（USART3 + DIR + 接收中断） */
#if BOARD_HAS_RS485
    RS485_Init();
#endif

    /* B 板的 NTC/MQ135/MQ2 都走 ADC1 */
#if BOARD_HAS_TEMP && BOARD_HAS_MQ135
    ADC1_Init();
#endif
}
