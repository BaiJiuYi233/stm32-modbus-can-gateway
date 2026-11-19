#ifndef __BSP_TMEP_H
#define __BSP_TMEP_H


#include "main.h"

/* ===================== NTC 参数 ===================== */
/* 分压电路里的固定电阻，单位 Ω */
#define FIXED_RESISTOR  10000.0f  /* 模块上的固定电阻，通常是 10K */

/* NTC 在 25°C 时的标称阻值，单位 Ω */
#define NTC_R25         10000.0f  /* 通常是 10K */

/* NTC 的 B 常数；模块是 3435 就改成 3435 */
#define NTC_B_VALUE     3950.0f

/* 启动时校准 ADC，必须在 ADC_Init 之后调用 */
#define Calib()    HAL_ADCEx_Calibration_Start(&hadc1)

/* NTC 最新采样值 */
typedef struct
{
	uint16_t tmpe_adc_raw;  /* PA4 的 ADC 原始值 0~4095 */
	float temperature;      /* 换算后的摄氏度 */
} tmpe_data;

/* 供 Modbus 和打印任务读取的全局数据 */
extern volatile tmpe_data g_sensor_data;

/**
  * @brief  ADC 原始值 -> NTC 摄氏度
  * @param  adc_value：0~4095
  * @retval 摄氏度
  */
float Calculate_Temperature(uint16_t adc_value);

#endif /* __BSP_TMEP_H */