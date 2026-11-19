#ifndef __BSP_MODBUS_H
#define __BSP_MODBUS_H

#include "main.h"
#include "FreeRTOS.h"

/* B 板的 Modbus 从站地址 */
#define MODBUS_SLAVE_ADDR   2

/* 寄存器数量：温度、ADC、状态、MQ2、MQ135、气压、预留 */
#define MODBUS_REG_COUNT    7

/* 解析寄存器的数量 */
#define SENSOR_NUM          3

/* 传感器状态的枚举 */
typedef enum {ST_OK = 0 , ST_LOW , ST_HIGH , ST_COUNT}Status;

/* 传感器的详细值 */
typedef struct 
{
	const char* name; //名字
	const char* unit; //数值单位
	float min; //支持最小值
	float max; //支持最大值
	float value; // 当前数值
	Status status; //状态
}Sensor;

/* 传感器更新次数 */
extern volatile uint16_t sensor_seq;

/* ------------------ 函数声明 ------------------ */

/* 初始化队列和寄存器表；在 RS485_Init() 之前调用 */
void Modbus_Slave_Init(void);

/* 在 Modbus 任务里处理一帧请求 */
void Modbus_Slave_Handle(const uint8_t *buf, uint16_t len);

/* 在 RS485 接收中断里调用：把整帧复制进队列
   返回值给 portYIELD_FROM_ISR 用 */
BaseType_t Modbus_Slave_RxFromISR(const uint8_t *buf, uint16_t len);

/* Modbus 从站任务：从队列取请求并处理 */
void Modbus_Task(void *pvParameters);

/* 标准 Modbus CRC16：低字节先发（主站/从站共用） */
uint16_t Modbus_CRC16(const uint8_t *buf, uint16_t len);

/* A 板主站任务：每秒读 B 板 0、1 号寄存器 */
void Modbus_Master_Task(void *pvParameters);

/* A 板 RS485 中断里保存 B 的回复 */
void Modbus_Master_RxFromISR(const uint8_t *buf, uint16_t len);

#endif /* __BSP_MODBUS_H */
