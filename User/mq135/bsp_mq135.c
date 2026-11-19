/**
  ******************************************************************************
  * @file       mq135.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      MQ135 空气质量传感器换算
  *
  * @note       模块输出 AO 电压，经 ADC 采样后换算成 Rs，
  *             再用 Rs/R0 和曲线估算空气质量。
  *             ppm 是估算值，不能当作实验室级 CO2 计。
  ******************************************************************************
  */

#include "mq135/bsp_mq135.h"

/* 最近一次 MQ135 的 ADC 原始值和换算结果，供 Modbus/打印读取 */
volatile mq135_data mq135_sensor_data;

/**
  * @brief  把 MQ135 的 ADC 原始值换算成估算 ppm
  * @param  adc_value：ADC 原始值，范围 0~4095
  * @retval 估算 ppm；低于 10 返回 0，高于 1000 返回 1000
  * @note   VC_MQ135 是模块供电/分压前的满量程电压；
  *         当前模块自带分压，按 5V 还原 AO 电压。
  *         MQ135 对多种气体都有响应，不能单靠一个值判断具体气体。
  */
float Get_Mq135_ppm(uint16_t adc_value)
{
    float vrl;       /* AO 还原后的模拟电压 */
    float Rs;        /* 当前传感器电阻，单位与 RL 一致 */
    float Mq135_ppm; /* 估算的气体浓度 */

    /* 1. ADC 原始值 -> AO 电压
       当前代码按模块分压后的 5V 满量程还原 */
    vrl = (float)adc_value / 4095.0f * VC_MQ135;

    /* 2. 分压公式反推传感器电阻
       VRL = VCC * RL / (Rs + RL)
       Rs  = (VCC - VRL) * RL / VRL */
    Rs = (VC_MQ135 - vrl) * RL_MQ135 / vrl;

    /* 3. 用 datasheet 的 log-log 曲线估算 ppm
       y = a * x^b，x = Rs / R0 */
    Mq135_ppm = A_MQ135 * pow(Rs / R0_MQ135, B_MQ135);

    /* 4. 限幅：低于检测下限按 0，高于上限按 1000 */
    if (Mq135_ppm < 10)
        return 0.0f;
    else if (Mq135_ppm > 1000)
        return 1000.0f;
    else
        return Mq135_ppm;
}
