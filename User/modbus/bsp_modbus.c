/**
  ******************************************************************************
  * @file       bsp_modbus.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      Modbus RTU：B 板从站 + A 板最小主站
  *
  * @note       从站：USART3 空闲中断 -> 队列 -> Modbus_Task -> 回复。
  *             主站：A 板 task1 -> 发 0x03 -> 等 B 回复 -> 打印。
  ******************************************************************************
  */

#include "modbus/bsp_modbus.h"
#include "board/board_select.h"
#include "rs485/bsp_rs485.h"
#include "temp/bsp_temp.h"
#include "mq135/bsp_mq135.h"
#include "mq2/bsp_mq2.h"
#include "queue.h"
#include "task.h"
#include <string.h>

volatile uint16_t sensor_seq = 0;

/**
  * @brief  标准 Modbus RTU CRC16
  * @param  buf：数据首地址
  * @param  len：参与 CRC 的字节数（不包含 CRC 本身）
  * @retval 16 位 CRC；发送时先发低字节，再发高字节
  * @note   主站和从站共用这一份，不要各写一份
  */
uint16_t Modbus_CRC16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    while (len--)
    {
        crc ^= *buf++;

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/* ======================== B 板：Modbus 从站 ======================== */

#if BOARD_MODBUS_SLAVE

#define MODBUS_QUEUE_LEN   4
#define MODBUS_FRAME_MAX   32

typedef struct
{
    uint16_t len;
    uint8_t  data[MODBUS_FRAME_MAX];
} modbus_frame_t;

static QueueHandle_t s_modbus_queue;
static uint16_t      s_regs[MODBUS_REG_COUNT];

/**
  * @brief  把传感器最新值填进 Modbus 寄存器表
  * @note   寄存器 0：温度 ×100（int16）
  *         寄存器 1：标志位（给发送次数计数）
  *         寄存器 2：MQ135空气质量传感器
  *         寄存器 3：MQ2烟雾传感器
  *					寄存器 /4/5/6：预留
  */
static void Modbus_Slave_UpdateRegs(void)
{
    s_regs[0] = (uint16_t)(int16_t)(g_sensor_data.temperature * 100.0f);
    s_regs[1] = sensor_seq;
    s_regs[2] = (uint16_t)mq135_sensor_data.mq135_ppm;
    s_regs[3] = (uint16_t)mq2_sensor_data.mq2_ppm;
    s_regs[4] = 0;
    s_regs[5] = 0;
    s_regs[6] = 0;
}

/**
  * @brief  发送一段字节（自动切 RS485 方向）
  * @note   只在任务上下文调用
  */
static void Modbus_SendBytes(uint8_t *buf, uint16_t len)
{
    RS485_DIR_TX();
    RS485_Send(buf, len);
    RS485_DIR_RX();
}

/**
  * @brief  回异常响应：地址 + (功能码|0x80) + 异常码 + CRC
  */
static void Modbus_SendException(uint8_t func, uint8_t code)
{
    uint8_t  resp[5];
    uint16_t crc;

    resp[0] = MODBUS_SLAVE_ADDR;
    resp[1] = func | 0x80;
    resp[2] = code;

    crc = Modbus_CRC16(resp, 3);
    resp[3] = crc & 0xFF;
    resp[4] = crc >> 8;

    Modbus_SendBytes(resp, 5);
}

/**
  * @brief  初始化 Modbus 从站
  * @note   必须在 RS485_Init() 之前调用
  */
void Modbus_Slave_Init(void)
{
    memset(s_regs, 0, sizeof(s_regs));

    s_modbus_queue = xQueueCreate(MODBUS_QUEUE_LEN, sizeof(modbus_frame_t));
    configASSERT(s_modbus_queue != NULL);

    printf("Modbus RTU slave: addr=%d, regs=%d\r\n",
           MODBUS_SLAVE_ADDR, MODBUS_REG_COUNT);
}

/**
  * @brief  RS485 中断里把整帧复制进队列
  * @param  buf：RS485_RxBuf
  * @param  len：本次空闲中断收到的长度
  * @retval pdTRUE 表示有更高优先级任务被唤醒
  */
BaseType_t Modbus_Slave_RxFromISR(const uint8_t *buf, uint16_t len)
{
    modbus_frame_t frame;
    BaseType_t     woken = pdFALSE;

    if (s_modbus_queue == NULL || buf == NULL || len < 4)
    {
        return pdFALSE;
    }

    if (len > MODBUS_FRAME_MAX)
    {
        len = MODBUS_FRAME_MAX;
    }

    frame.len = len;
    memcpy(frame.data, buf, len);

    xQueueSendFromISR(s_modbus_queue, &frame, &woken);

    return woken;
}

/**
  * @brief  处理一帧 Modbus RTU 请求（只支持 0x03）
  * @param  buf：完整请求帧（含 CRC）
  * @param  len：请求帧长度
  * @note   地址不对不回答；CRC 错不回答；功能码不支持回异常 01；
  *         寄存器越界回异常 02。
  */
void Modbus_Slave_Handle(const uint8_t *buf, uint16_t len)
{
    uint16_t rx_crc;
    uint16_t start;
    uint16_t qty;
    uint8_t  func;

    if (buf == NULL || len < 8)
    {
        return;
    }

    if (buf[0] != MODBUS_SLAVE_ADDR)
    {
        return;
    }

    rx_crc = (uint16_t)buf[len - 2] | ((uint16_t)buf[len - 1] << 8);
    if (rx_crc != Modbus_CRC16(buf, len - 2))
    {
        return;
    }

    func = buf[1];

    if (func != 0x03)
    {
        Modbus_SendException(func, 0x01);
        return;
    }

    if (len != 8)
    {
        Modbus_SendException(func, 0x03);
        return;
    }

    start = ((uint16_t)buf[2] << 8) | buf[3];
    qty   = ((uint16_t)buf[4] << 8) | buf[5];

    if (qty == 0 || qty > MODBUS_REG_COUNT ||
        start + qty > MODBUS_REG_COUNT)
    {
        Modbus_SendException(func, 0x02);
        return;
    }

    Modbus_Slave_UpdateRegs();

    uint8_t  resp[5 + 2 * MODBUS_REG_COUNT];
    uint16_t n = 0;

    resp[n++] = MODBUS_SLAVE_ADDR;
    resp[n++] = func;
    resp[n++] = (uint8_t)(qty * 2);

    for (uint16_t i = 0; i < qty; i++)
    {
        uint16_t value = s_regs[start + i];
        resp[n++] = value >> 8;
        resp[n++] = value & 0xFF;
    }
		
    uint16_t crc = Modbus_CRC16(resp, n);
    resp[n++] = crc & 0xFF;
    resp[n++] = crc >> 8;

    Modbus_SendBytes(resp, n);
}

/**
  * @brief  Modbus 从站任务：从队列取请求，处理并回复
  */
void Modbus_Task(void *pvParameters)
{
    modbus_frame_t frame;

    (void)pvParameters;

    for (;;)
    {
        if (xQueueReceive(s_modbus_queue, &frame, portMAX_DELAY) == pdTRUE)
        {
            Modbus_Slave_Handle(frame.data, frame.len);
        }
    }
}

#endif /* BOARD_MODBUS_SLAVE */

/* ======================== A 板：Modbus 主站 ======================== */

#if BOARD_MODBUS_MASTER

#define MASTER_RX_MAX   32

static uint8_t          s_master_rx_buf[MASTER_RX_MAX];
static volatile uint16_t s_master_rx_len;
static volatile uint8_t  s_master_rx_flag;



/**
  * @brief  A 板 RS485 中断里保存 B 的回复
  * @note   只复制，不解析；解析在 Modbus_Master_Task 里做
  */
void Modbus_Master_RxFromISR(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len < 4)
    {
        return;
    }

    if (len > MASTER_RX_MAX)
    {
        len = MASTER_RX_MAX;
    }

    memcpy(s_master_rx_buf, buf, len);
    s_master_rx_len  = len;
    s_master_rx_flag = 1;
}
/**
  * @brief  按传感器表里的 min/max 判断状态
  * @param  s：传感器描述和当前值
  * @retval ST_OK / ST_LOW / ST_HIGH
  * @note   当前用 <= / >=，正好等于边界也算超范围；
  *         如果边界值应算正常，改成 < / >。
  */
Status check(const Sensor* s)
{
  if(s->value <= s->min) return ST_LOW;
	if(s->value >= s->max) return ST_HIGH;
	return ST_OK;
}

/**
  * @brief  A 板 Modbus 主站：每秒读 B 板 0~3 号寄存器
  * @note   请求：02 03 00 00 00 04 CRC
  *         回复：02 03 08 [温度][序号][MQ135][MQ2] CRC
  */
void Modbus_Master_Task(void *pvParameters)
{
    uint8_t  req[8];
    uint8_t  resp[MASTER_RX_MAX];
    uint16_t resp_len;

    (void)pvParameters;

    printf("Modbus master: poll B addr=%d\r\n", MODBUS_SLAVE_ADDR);

    for (;;)
    {
        /* 1. 组请求：读 5 个保持寄存器 */
        req[0] = MODBUS_SLAVE_ADDR; //设备地址
        req[1] = 0x03; //功能码
        req[2] = 0x00; //从第几个读取
        req[3] = 0x00;
        req[4] = 0x00; //读取几个
        req[5] = 0x04;

        uint16_t crc = Modbus_CRC16(req, 6);
        req[6] = crc & 0xFF; //CRC效验
        req[7] = crc >> 8;

        /* 2. 清掉上一次回复，再发请求 */
        s_master_rx_flag = 0;
        s_master_rx_len  = 0;

        RS485_DIR_TX();
        RS485_Send(req, sizeof(req));
        RS485_DIR_RX();

        /* 3. 等 B 回复，最多 100ms */
        uint32_t waited = 0;
        while (s_master_rx_flag == 0 && waited < 100)
        {
            vTaskDelay(1);
            waited++;
        }

        if (s_master_rx_flag == 0)
        {
            printf("B timeout\r\n");
            vTaskDelay(1000);
            continue;
        }

        /* 4. 把中断里的回复复制出来，避免下一帧覆盖 */
        taskENTER_CRITICAL();
        resp_len = s_master_rx_len;
        if (resp_len > sizeof(resp))
        {
            resp_len = sizeof(resp);
        }
        memcpy(resp, s_master_rx_buf, resp_len);
        s_master_rx_flag = 0;
        taskEXIT_CRITICAL();

        /* 5. 基本校验 */
        if (resp_len < 5 || resp[0] != MODBUS_SLAVE_ADDR)
        {
            printf("B bad addr/len\r\n");
            vTaskDelay(1000);
            continue;
        }

        uint16_t rx_crc = (uint16_t)resp[resp_len - 2] |
                          ((uint16_t)resp[resp_len - 1] << 8);

        if (rx_crc != Modbus_CRC16(resp, resp_len - 2))
        {
            printf("B bad crc\r\n");
            vTaskDelay(1000);
            continue;
        }

        if (resp[1] == 0x83)
        {
            printf("B exception: 0x%02X\r\n", resp[2]);
            vTaskDelay(1000);
            continue;
        }

        if (resp[1] != 0x03 || resp_len != (uint16_t)(5 + resp[2]) ||
            resp[2] < 8)
        {
            printf("B bad response\r\n");
            vTaskDelay(1000);
            continue;
        }

        /* 6. 解析 4 个 16 位寄存器：高字节在前 */
				/* 打印用的传感器表；SENSOR_NUM 必须和行数一致。
				   这里只负责“怎么显示”，不负责 Modbus 寄存器映射。 */
				Sensor sensors[] = {
				{"温度", "C" , -20.0f , 100.0f , 0 , ST_OK},
				{"空气", "ppm" , 0.0f , 1000.0f , 0 , ST_OK},
				{"烟雾", "ppm" , 0.0f , 10000.0f , 0 , ST_OK},
};
				
        uint16_t temp_x100 			= ((uint16_t)resp[3] << 8) | resp[4];
        uint16_t seq_raw    		= ((uint16_t)resp[5] << 8) | resp[6];
				uint16_t mq135_ppm  		= ((uint16_t)resp[7] << 8) | resp[8];
				uint16_t mq2_ppm        = ((uint16_t)resp[9] << 8) | resp[10];

        int32_t  temp_signed  = (int16_t)temp_x100;
        uint32_t temp_abs     = (temp_signed < 0) ?
                               (uint32_t)(-temp_signed) :
                               (uint32_t)temp_signed;
				
				sensors[0].value = (float)temp_abs/ 100.0f;
				sensors[1].value = (float)mq135_ppm;
				sensors[2].value = (float)mq2_ppm;
				
				static uint16_t last_sep = 0;
				static uint8_t last_sep_num = 0;
				
				if (seq_raw != last_sep) 
				{
					last_sep = seq_raw;
					last_sep_num = 0;
				}
				else
				{
					if (last_sep_num >= 3)
					{
						printf("B not ready\r\n");
						vTaskDelay(1000);
						continue;
					}
					else
					{
					 last_sep_num++;
					}
				}
				
			const char* status_str[ST_COUNT] = {"正常" , "低于量程" , "高于量程"};
			
			for (int i = 0; i < SENSOR_NUM; i++)
			{
				sensors[i].status = check(&sensors[i]);
				printf("%s:%.2f%s[%s]\r\n" , 
				sensors[i].name , sensors[i].value , sensors[i].unit , status_str[sensors[i].status]);
			}
			printf("计数：%u\r\n",seq_raw);
			
        vTaskDelay(1000);
    }
}

#endif /* BOARD_MODBUS_MASTER */
