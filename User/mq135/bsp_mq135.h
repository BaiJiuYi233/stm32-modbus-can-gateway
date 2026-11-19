#ifndef __BSP_MQ135_H
#define __BSP_MQ135_H

#include "main.h"

/* ===================== MQ135 参数 ===================== */
/* 模块负载电阻，单位和 R0 保持一致（kΩ） */
#define RL_MQ135      1.0f     /* 模块上 RL = 1k */
/* 干净空气下标定得到的基准电阻；不同模块/环境要重新标定 */
#define R0_MQ135      2.0f
/* 模块供电/分压前满量程；当前模块自带分压，按 5V 还原 AO */
#define VC_MQ135      5.0f
/* datasheet 曲线 y = a * x^b 的系数 */
#define A_MQ135       4.17f
#define B_MQ135      -2.28f

/* MQ135 的最新采样值 */
typedef struct
{
	uint16_t mq135_adc_raw;   /* ADC 原始值 0~4095 */
	float mq135_ppm;          /* 估算空气质量浓度 */
} mq135_data;

/* 供 Modbus 和打印任务读取的全局数据 */
extern volatile mq135_data mq135_sensor_data;

/**
  * @brief  ADC 原始值 -> MQ135 估算 ppm
  * @param  adc_value：0~4095
  * @retval 估算 ppm，超出范围时被限幅
  */
float Get_Mq135_ppm(uint16_t adc_value);

#endif
