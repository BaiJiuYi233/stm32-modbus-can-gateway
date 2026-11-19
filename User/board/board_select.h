#ifndef BOARD_SELECT_H
#define BOARD_SELECT_H

/**
  ******************************************************************************
  * @file       board_select.h
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      A/B/C 三块板的角色选择
  *
  * @note       Keil 每个 target 只定义 BOARD_A / BOARD_B / BOARD_C 中的一个；
  *             这里再根据宏裁剪 CAN、RS485、传感器和任务。
  ******************************************************************************
  */

/* 三个宏只能有一个；一个都没写、或写了两个，直接编译失败 */
#if (defined(BOARD_A) + defined(BOARD_B) + defined(BOARD_C)) != 1
#error "Keil 的 Define 里只选一个：BOARD_A / BOARD_B / BOARD_C"
#endif

/* ===================== A 板：Modbus 主站 + CAN 接收 ===================== */
#if defined(BOARD_A)
  #define BOARD_NAME          "A"
  #define BOARD_HAS_CAN       1   /* A 板有 CAN，用于接收 C 的报警帧 */
  #define BOARD_HAS_CAN_RX    1   /* 创建 CAN_RX_Task */
  #define BOARD_HAS_RS485     1   /* A 板有 RS485，作为 Modbus 主站 */
  #define BOARD_MODBUS_MASTER 1   /* 创建 Modbus 主站任务 */
  #define BOARD_MODBUS_SLAVE  0

/* ===================== B 板：Modbus 从站 + 传感器 ===================== */
#elif defined(BOARD_B)
  #define BOARD_NAME          "B"
  #define BOARD_HAS_CAN       0
  #define BOARD_HAS_RS485     1   /* B 板有 RS485，作为 Modbus 从站 */
  #define BOARD_HAS_TEMP      1   /* NTC 温度 */
  #define BOARD_HAS_MQ135     1   /* MQ135 空气质量 */
  #define BOARD_HAS_MQ2       1   /* MQ2 烟雾 */
  #define BOARD_MODBUS_MASTER 0
  #define BOARD_MODBUS_SLAVE  1   /* 创建 Modbus_Task */

/* ===================== C 板：CAN 报警节点 ===================== */
#elif defined(BOARD_C)
  #define BOARD_NAME          "C"
  #define BOARD_HAS_CAN       1
  #define BOARD_HAS_CAN_TX    1   /* 创建 can_tx_task，周期发 0x180 */
  #define BOARD_HAS_RS485     0   /* C 板没有 RS485 */
  #define BOARD_HAS_BUZZ      1   /* 火焰报警蜂鸣器 */
  #define BOARD_HAS_FLAME     1   /* 火焰传感器 DO */
  #define BOARD_MODBUS_MASTER 0
  #define BOARD_MODBUS_SLAVE  0
#endif

/************ 函数声明 ************/
/**
  * @brief  按当前 BOARD_* 宏初始化板级外设
  * @param  无
  * @retval 无
  */
void Board_Init(void);

#endif /* BOARD_SELECT_H */
