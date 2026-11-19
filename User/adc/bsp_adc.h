#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "main.h"

/* ADC1 句柄：在 bsp_adc.c 里定义，其他文件用 extern 访问 */
extern ADC_HandleTypeDef hadc1;

/* ------------------------- ADC 引脚映射 ------------------------- */
/* PA4：ADC1_IN4，热敏电阻分压 */
#define TEMP_Pin              GPIO_PIN_4
#define TEMP__GPIO_Port       GPIOA

/* PA5：ADC1_IN5，MQ135 空气质量模块的模拟输出 */
#define MQ135_Pin             GPIO_PIN_5
#define MQ135_GPIO_Port       GPIOA

/* PA6：ADC1_IN6，MQ2 烟雾质量模块的模拟输出 */

#define MQ2_Pin               GPIO_PIN_6
#define MQ2_GPIO_Port         GPIOA

/* 说明：
 * 目前配了 PA4/PA5 两个模拟脚。
 * PA4 是默认通道；adc_read_channel() 可以按通道号读 PA4/PA5/PA6。
 * 如果以后加 MQ2，建议接 PA6（ADC1_IN6），并做两件事：
 *   1. 在 HAL_ADC_MspInit() 的 GPIO 初始化里加上 GPIO_PIN_6；
 *   2. 读取时用 adc_read_channel(ADC_CHANNEL_6)。
 * 注意：PA6 不要同时被别的外设占用。
 */

/* ------------------------- 函数声明 ------------------------- */
/* 初始化 ADC1：单通道、默认 PA4 */
void ADC1_Init(void);

/* 读单个通道：选通道 -> Rank1 -> 启动 -> 等待 -> 读值 -> 停止 */
uint16_t adc_read_channel(uint32_t channel);

/* HAL 底层初始化：HAL_ADC_Init() 会自动调用，一般不用手动调 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle);

/* HAL 底层反初始化：关闭 ADC1 时钟、释放 PA4/PA5 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle);

#endif /* __BSP_ADC_H */
