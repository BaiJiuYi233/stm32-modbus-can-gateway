#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"

/* ===================== CAN 外部句柄/缓冲区 ===================== */
extern CAN_HandleTypeDef hcan;     /* CAN1 句柄 */
extern uint8_t CAN_RxBuf[8];       /* 经典 CAN 单帧最大 8 字节 */

/* ===================== CAN 引脚 ===================== */
#define CAN_RX_Pin        GPIO_PIN_11      /* PA11 = CAN1_RX */
#define CAN_RX_GPIO_Port  GPIOA
#define CAN_TX_Pin        GPIO_PIN_12      /* PA12 = CAN1_TX */
#define CAN_TX_GPIO_Port  GPIOA

/* ===================== CAN 接收队列 ===================== */

/* 队列长度 */
#define CAN_QUEUE_LEN     4

/* 中断收到一帧后复制到该队列，任务侧再取出，避免在 ISR 里打印 */
extern QueueHandle_t can_queue;

/* 队列创建失败标志：1 = 失败，接收任务不应再操作队列 */
extern volatile uint8_t queue_err;

/* 队列里传递的一帧 CAN 数据 */
typedef struct
{
    uint32_t id;        /* 标准帧 ID 或扩展帧 ID */
    uint8_t  dlc;       /* 数据长度 0~8 */
    uint8_t  data[8];   /* 数据内容 */
} can_msg_t;

/* ===================== 函数声明 ===================== */
void CAN_Config(void);          /* 配置 CAN1 公共参数 */
void CAN_Filter_Config(void);   /* 配置接收滤波器 */
void CAN_Init(void);            /* 初始化 CAN、队列和接收中断 */
void CAN_Send(uint32_t StdId, uint8_t *pData, uint8_t Len);

/* HAL 的 FIFO0 接收回调：只负责复制入队，不做打印/解析 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);

#endif /* __BSP_CAN_H */
