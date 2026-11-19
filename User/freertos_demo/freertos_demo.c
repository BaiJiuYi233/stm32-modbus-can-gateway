/**
  ******************************************************************************
  * @file       freertos_demo.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      FreeRTOS配置
  *
  ******************************************************************************
	*/
#include "freertos_demo/freertos_demo.h"
#include "led/bsp_led.h"
#include "usart/bsp_usart.h"
#include "adc/bsp_adc.h"
#include "can/bsp_can.h"
#include "rs485/bsp_rs485.h"
#include "temp/bsp_temp.h"
#include "mq135/bsp_mq135.h"
#include "mq2/bsp_mq2.h"
#include "buzz/bsp_buzz.h"
#include "flame/bsp_flame.h"
#include "board/board_select.h"
#include "modbus/bsp_modbus.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


/* START_TASK 任务配置 */
#define START_TACK_PRIO       1
#define START_TACK_STACK_SIZE 128
TaskHandle_t start_tack_handler;
void start_task( void * pvParameters );

/* TASK1：RS485 发送 */
#define RS485_TX_PRIO       2
#define RS485_TX_STACK_SIZE 256
TaskHandle_t rs485_tx_handler;
void rs485_tx_task( void * pvParameters );

/* TASK2：CAN 发送 */
#define CAN_TX_PRIO       2
#define CAN_TX_STACK_SIZE 256
TaskHandle_t can_tx_handler;
void can_tx_task( void * pvParameters );

/* TASK3：传感器采样 */
#define SENSOR_TACK_PRIO       3
#define SENSOR_TACK_STACK_SIZE 256
TaskHandle_t Sensor_tack_handler;
void Sensor_task( void * pvParameters );

/* TASK4：蜂鸣器和火焰传感器（测试中） */
#define TACK4_PRIO       5
#define TACK4_STACK_SIZE 128
TaskHandle_t tack4_handler;
void task4( void * pvParameters );

/* TASK5：Modbus RTU 从站（B 板） */
#define MODBUS_TASK_PRIO       3
#define MODBUS_TASK_STACK_SIZE 256
TaskHandle_t modbus_task_handler;
void Modbus_Task( void * pvParameters );

/* TASK6：CAN报警帧接收（A 板） */
#define CAN_RX_TASK_PRIO       3
#define CAN_RX_TASK_STACK_SIZE 256
TaskHandle_t CAN_RX_Task_handler;
void CAN_RX_Task( void * pvParameters );

void freertos_demo(void)
{
		
    xTaskCreate( (TaskFunction_t        )   start_task,
                 (char *                )   "start_task",
                 (configSTACK_DEPTH_TYPE)   START_TACK_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   START_TACK_PRIO,
                 (TaskHandle_t *        )   &start_tack_handler   );

    vTaskStartScheduler();
}

/**
  * @brief  按板型创建业务任务，创建完成后删除自己
  * @param  pvParameters：未使用
  * @retval 无
  * @note   用 BOARD_* 宏裁剪，A/B/C 三块板只创建各自需要的任务。
  */
void start_task( void * pvParameters )
{
    taskENTER_CRITICAL();

#if BOARD_MODBUS_MASTER
    xTaskCreate( (TaskFunction_t        )   rs485_tx_task,
                 (char *                )   "rs485_tx_task",
                 (configSTACK_DEPTH_TYPE)   RS485_TX_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   RS485_TX_PRIO,
                 (TaskHandle_t *        )   &rs485_tx_handler   );
#endif

#if BOARD_HAS_TEMP && BOARD_HAS_MQ135 && BOARD_HAS_MQ2
    xTaskCreate( (TaskFunction_t        )   Sensor_task,
                 (char *                )   "Sensor_task",
                 (configSTACK_DEPTH_TYPE)   SENSOR_TACK_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   SENSOR_TACK_PRIO,
                 (TaskHandle_t *        )   &Sensor_tack_handler   );
#endif
								 


#if BOARD_MODBUS_SLAVE
    xTaskCreate( (TaskFunction_t        )   Modbus_Task,
                 (char *                )   "modbus",
                 (configSTACK_DEPTH_TYPE)   MODBUS_TASK_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   MODBUS_TASK_PRIO,
                 (TaskHandle_t *        )   &modbus_task_handler   );
#endif

#if BOARD_HAS_CAN_TX
    xTaskCreate( (TaskFunction_t        )   can_tx_task,
                 (char *                )   "can_tx_task",
                 (configSTACK_DEPTH_TYPE)   CAN_TX_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   CAN_TX_PRIO,
                 (TaskHandle_t *        )   &can_tx_handler   );
#endif
								 
#if BOARD_HAS_BUZZ
    xTaskCreate( (TaskFunction_t        )   task4,
                 (char *                )   "task4",
                 (configSTACK_DEPTH_TYPE)   TACK4_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   TACK4_PRIO,
                 (TaskHandle_t *        )   &tack4_handler   );
#endif
								 
#if BOARD_HAS_CAN_RX
    xTaskCreate( (TaskFunction_t        )   CAN_RX_Task,
                 (char *                )   "CAN_RX_Task",
                 (configSTACK_DEPTH_TYPE)   CAN_RX_TASK_STACK_SIZE,
                 (void *                )   NULL,
                 (UBaseType_t           )   CAN_RX_TASK_PRIO,
                 (TaskHandle_t *        )   &CAN_RX_Task_handler   );
#endif

    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}


/* 任务一：A 板 Modbus 主站（B 板从站不创建这个任务） */
#if BOARD_MODBUS_MASTER
/**
  * @brief  A 板 RS485 主站任务入口
  * @param  pvParameters：未使用
  * @retval 无
  * @note   实际轮询逻辑在 Modbus_Master_Task() 中，本函数不会返回。
  */
void rs485_tx_task( void * pvParameters )
{
    /* 这个函数不会返回：里面每秒发一次 0x03 请求 */
    Modbus_Master_Task(pvParameters);
}
#endif


/* 任务二：CAN 发送 */
#if BOARD_HAS_CAN_TX

/**
  * @brief  C 板 CAN 报警帧发送任务
  * @param  pvParameters：未使用
  * @retval 无
  * @note   每 1 秒发送一帧 0x180，data[3] 携带火焰标志。
  */
void can_tx_task( void * pvParameters )
{
    uint8_t CanData[8];
		uint32_t Can_ID = 0x180;
	  uint8_t seq = 0;

    for (;;) 
		{
       /* 0x180 报警帧：
          [0]=C 节点 [1]=报警类型 [2]=序号 [3]=火焰标志 */
       CanData[0] = 0x03;
			 CanData[1] = 0x01;
			 CanData[2] = seq;
			 CanData[3] = flame_flags;
			 CanData[4] = 0x00;
			 CanData[5] = 0x00;
			 CanData[6] = 0x00;
			 CanData[7] = 0x00;
			
			 CAN_Send(Can_ID , CanData , 8);
			 seq++;   /* 帧序号递增，A 可用来判断旧帧 */
			
       vTaskDelay(1000);
    }
}

#endif

/* 任务六：CAN任务接收 */
#if BOARD_HAS_CAN_RX
/**
  * @brief  A 板 CAN 报警帧接收任务
  * @param  pvParameters：未使用
  * @retval 无
  * @note   从队列取帧，检查节点/类型/序号，打印数据；
  *         不在 CAN 中断里 printf。
  */
void CAN_RX_Task( void * pvParameters )
{
	 can_msg_t can_data_rx;
	 static uint8_t last_sep;
	 static uint8_t have_last;
	for(;;)
	{
		/* 队列创建失败时，没有队列可读，直接删除本任务 */
		if(queue_err == 0)
		{
		  /* 没有帧时阻塞在这里，不占 CPU */
		  if (xQueueReceive(can_queue , &can_data_rx , portMAX_DELAY) == pdTRUE)
		  {
				if (can_data_rx.id == 0x180)
				{	
					/* 只处理 C 节点发来的报警类型 */
					if (can_data_rx.data[0] != 0x03)continue;
				
					if (can_data_rx.data[1] != 0x01)continue;
				
					/* 第一帧先记录序号，不判旧 */
					if (have_last == 0)
					{
						have_last++; 
						last_sep = can_data_rx.data[2];
					}
					else if (can_data_rx.data[2] != last_sep)
					{
						/* 序号变了，说明是新帧 */
						last_sep = can_data_rx.data[2];
					}
					else
					{
						printf("这是旧数据\r\n");
						continue;
					}
				
					if (can_data_rx.data[3] == 0x01)
					{
						printf("火焰传感器触发和蜂鸣器\r\n");
					}
				}
			/* 打印每一帧的数据，调试时能确认 CAN 真的收到了 */
				for(int i = 0; i < can_data_rx.dlc; i++)
				{
					printf("%.2x ",can_data_rx.data[i]);
				}
				printf("\r\n");
			}
		}
		else
		{
			 vTaskDelete(NULL);
		}
	}

}
#endif


/* 任务三：采样传感器，算出温度后交给 task1 发送 */
#if BOARD_HAS_TEMP && BOARD_HAS_MQ135 && BOARD_HAS_MQ2

/**
  * @brief  B 板传感器采集任务
  * @param  pvParameters：未使用
  * @retval 无
  * @note   一个任务顺序读 PA4/PA5/PA6，避免多个任务同时操作 ADC；
  *         每 500ms 更新一次温度和两个 MQ 传感器。
  */
void Sensor_task( void * pvParameters )
{
    Calib();
    while (1)
    {
        /* 同一个任务顺序读三个 ADC 通道，避免多任务抢同一个 ADC */
        g_sensor_data.tmpe_adc_raw = adc_read_channel(ADC_CHANNEL_4); //热敏传感器
        g_sensor_data.temperature = Calculate_Temperature(g_sensor_data.tmpe_adc_raw);
			
				mq135_sensor_data.mq135_adc_raw = adc_read_channel(ADC_CHANNEL_5);//空气质量传感器
				mq135_sensor_data.mq135_ppm = Get_Mq135_ppm(mq135_sensor_data.mq135_adc_raw);
			
				mq2_sensor_data.mq2_adc_raw = adc_read_channel(ADC_CHANNEL_6); //烟雾传感器
			  mq2_sensor_data.mq2_ppm = Get_Mq2_ppm(mq2_sensor_data.mq2_adc_raw);
				sensor_seq++;   /* 每完成一轮采样，序号 +1 */
        vTaskDelay(500);
    }
}

#endif



/* 任务四：触发火焰传感器和蜂鸣器 */
#if BOARD_HAS_BUZZ && BOARD_HAS_FLAME

/**
  * @brief  C 板火焰检测和本地蜂鸣器任务
  * @param  pvParameters：未使用
  * @retval 无
  * @note   50ms 轮询一次 DO；火焰有效时置 flame_flags 并开蜂鸣器。
  */
void task4( void * pvParameters )
{
	
    while (1) 
		{
			/* 火焰模块通常是低电平有效；以实际模块手册为准 */
			if (flame_Read() == GPIO_PIN_RESET)
			{
				 BUZZ_ON();
				 flame_flags = 1;
			}
			else
			{
				 BUZZ_OFF();
				 flame_flags = 0;
			}
        vTaskDelay(50);   /* 50ms 轮询，兼顾响应和 CPU */
    }
}

#endif
