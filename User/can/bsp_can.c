/**
  ******************************************************************************
  * @file       bsp_can.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      CAN1 初始化、发送、接收队列
  *
  * @note       接收中断只把 CAN 帧复制进队列；
  *             打印/解析在 CAN_RX_Task 里做，避免在 ISR 中 printf。
  ******************************************************************************/
	
#include "can/bsp_can.h"
#include "usart/bsp_usart.h"
#include "led/bsp_led.h"
#include "board/board_select.h"
#include "FreeRTOS.h"
#include "queue.h"

#if BOARD_HAS_CAN

CAN_HandleTypeDef hcan;        /* CAN1 外设句柄 */

QueueHandle_t can_queue;       /* 接收中断 -> 任务 的 CAN 帧队列 */

volatile uint8_t queue_err = 0; /* 1 = 队列创建失败，接收任务不要再读 */


/**
  * @brief  初始化 CAN 外设
  * @note   配置 CAN 波特率、模式及相关功能参数
  * @param  无
  * @retval 无
  */
void CAN_Config(void)
{
    hcan.Instance = CAN1;                           // 指定 CAN 实例为 CAN1
    hcan.Init.Prescaler = 4;                        // 配置 CAN 波特率分频
    hcan.Init.Mode = CAN_MODE_NORMAL;               // 设置 CAN 工作模式为正常模式
    hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;         // 设置同步跳转宽度
    hcan.Init.TimeSeg1 = CAN_BS1_5TQ;              // 设置时间段1
    hcan.Init.TimeSeg2 = CAN_BS2_3TQ;              // 设置时间段2
    hcan.Init.TimeTriggeredMode = DISABLE;         // 禁用时间触发模式
    hcan.Init.AutoBusOff = ENABLE;                  // 启用自动总线关闭功能
    hcan.Init.AutoWakeUp = ENABLE;                  // 启用自动唤醒功能
    hcan.Init.AutoRetransmission = ENABLE;         // 启用自动重传功能
    hcan.Init.ReceiveFifoLocked = DISABLE;         // 禁止接收 FIFO 锁定
    hcan.Init.TransmitFifoPriority = DISABLE;      // 禁止发送 FIFO 优先级

    if (HAL_CAN_Init(&hcan) != HAL_OK)              // 初始化 CAN 外设
    {
        Error_Handler();                            // 初始化失败，执行错误处理
    }
}

/**
  * @brief  CAN MSP 初始化，配置 CAN 引脚和中断
  * @param  canHandle 指向 CAN 外设句柄的指针
  * @retval 无
  */
void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(canHandle->Instance == CAN1)
    {
			__HAL_RCC_CAN1_CLK_ENABLE();               // 使能 CAN1 外设时钟
			__HAL_RCC_GPIOA_CLK_ENABLE();              // 使能 GPIOA 时钟

			/* 配置 CAN RX 引脚 */
			GPIO_InitStruct.Pin = CAN_RX_Pin;                 // 设置 RX 引脚
			GPIO_InitStruct.Mode = GPIO_MODE_INPUT;           // 配置为输入模式
			GPIO_InitStruct.Pull = GPIO_NOPULL;               // 无上下拉
			HAL_GPIO_Init(CAN_RX_GPIO_Port, &GPIO_InitStruct); // 初始化 RX 引脚对应的 GPIO 端口

			/* 配置 CAN TX 引脚 */
			GPIO_InitStruct.Pin = CAN_TX_Pin;                 // 设置 TX 引脚
			GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;           // 配置为复用推挽输出
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;     // 设置高速输出
			HAL_GPIO_Init(CAN_TX_GPIO_Port, &GPIO_InitStruct); // 初始化 TX 引脚对应的 GPIO 端口

			/* 配置 CAN1 RX1 中断 */
			HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 5 , 0); // 设置中断优先级为 0
			HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);         // 使能 CAN1 RX1 中断
    }
}

/**
  * @brief  CAN MSP 反初始化，关闭 CAN 外设时钟和中断
  * @param  canHandle 指向 CAN 外设句柄的指针
  * @retval 无
  */
void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{
    if(canHandle->Instance == CAN1)
    {
        __HAL_RCC_CAN1_CLK_DISABLE();             // 禁用 CAN1 外设时钟
        HAL_GPIO_DeInit(GPIOA, CAN_RX_Pin|CAN_TX_Pin); // 反初始化 CAN RX/TX 引脚
        HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);       // 禁用 CAN1 RX1 中断
    }
}

/**
  * @brief  配置 CAN 滤波器
  * @note   设置 CAN 接收滤波器参数，仅允许接收标准 ID 为 0x180 的消息
  * @param  无
  * @retval 无
  */
void CAN_Filter_Config(void)
{
    CAN_FilterTypeDef can_filter = {0};           // 定义 CAN 滤波器结构体并清零

    can_filter.FilterBank = 0;                     // 使用第 0 个滤波器组
    can_filter.FilterMode = CAN_FILTERMODE_IDMASK; // 设置滤波器模式为掩码模式
    can_filter.FilterScale = CAN_FILTERSCALE_32BIT; // 32 位滤波器
    can_filter.FilterIdHigh     = (0x180 << 5);    // 高 16 位 ID，标准帧 ID 左移 5 位
    can_filter.FilterIdLow      = 0x0000;          // 标准帧下低 16 位为 0
    can_filter.FilterMaskIdHigh = (0x7FF << 5);    // 高 16 位掩码，仅匹配 11 位标准 ID
    can_filter.FilterMaskIdLow  = 0x0000;          // 低 16 位掩码为 0
    can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0; // 指定接收 FIFO0
    can_filter.FilterActivation = ENABLE;          // 激活滤波器

    if(HAL_CAN_ConfigFilter(&hcan, &can_filter) != HAL_OK) // 配置滤波器
    {
        Error_Handler();                           // 配置失败，执行错误处理
    }
}

/**
 * @brief  完整初始化CAN的功能
 * @param  无
 * @retval 无
 */
void CAN_Init(void)
{
    /* 初始化 CAN 配置 */
    CAN_Config();
    
    /* 配置 CAN 过滤器 */
    CAN_Filter_Config();
	
		/* 先创建队列，再开接收中断；
		   否则中断先来时会拿到空队列 */
		can_queue = xQueueCreate(CAN_QUEUE_LEN , sizeof(can_msg_t));
	
	  /* 队列创建失败：点亮红灯并置错误标志 */
	  if(can_queue == NULL) 
		{
			printf("can_queue创建失败");
			R_LED_ON_ONLY();
			queue_err = 1;
		}
	
		/* 启动 CAN 外设 */
		HAL_CAN_Start(&hcan);

		/* 激活接收中断（FIFO0 消息挂起中断） */
		HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
	
	  /* 亮绿灯 */
		G_LED_ON();
}



/**
  * @brief  发送 CAN 数据
  * @param  StdId 标准 ID
  * @param  pData 指向发送数据的指针
  * @param  Len   发送数据长度（最大 8 字节）
  * @retval 无
  */
void CAN_Send(uint32_t StdId, uint8_t *pData, uint8_t Len)
{
    CAN_TxHeaderTypeDef txHeader;                 // 定义 CAN 发送报文头
    uint32_t txMailbox;                           // 保存发送邮箱编号

    txHeader.StdId = StdId;                       // 设置标准 ID
    txHeader.IDE = CAN_ID_STD;                    // 标准帧
    txHeader.RTR = CAN_RTR_DATA;                  // 数据帧
    txHeader.DLC = Len;                           // 数据长度

    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, pData, &txMailbox) == HAL_OK)
    {
        printf("CAN 发送数据成功! \r\n");
        printf("发送的报文信息如下： \r\n");
        printf("标准 ID：0x%03X \r\n", txHeader.StdId);
			  if(txHeader.RTR == CAN_RTR_DATA)
			  printf("帧类型：数据帧 \r\n");
				else
				printf("帧类型：远程帧 \r\n");	
    }
}

/**
  * @brief  CAN 接收 FIFO0 消息回调（中断上下文）
  * @param  hcan：CAN 句柄
  * @retval 无
  * @note   这里只读取、复制、入队，不做 printf，也不做协议解析。
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;  /* 接收报文头（ID/DLC/帧类型） */
	
	  can_msg_t can;                 /* 一帧的临时结构体，入队时会复制 */
	  BaseType_t woken = pdFALSE;    /* 是否有更高优先级任务被唤醒 */
	  BaseType_t CAN_Queue;          /* 入队结果，当前未使用 */

    /* 1. 从 FIFO0 取出一帧，数据直接放进 can.data */
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, can.data);

    /* 2. 记录 ID 和数据长度 */
    can.id = rxHeader.StdId;
    can.dlc = rxHeader.DLC;
		
	/* 3. 队列可用才入队；队列满/失败时不在中断里等待 */
	if (queue_err == 0)
	{
		CAN_Queue = xQueueSendFromISR(can_queue , &can , &woken);
	
		/* 如果唤醒了更高优先级任务，中断结束时切换 */
		portYIELD_FROM_ISR(woken);
	}

}
#endif



/*****************************END OF FILE***************************************/
