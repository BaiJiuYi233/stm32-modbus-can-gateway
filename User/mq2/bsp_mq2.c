/**
  ******************************************************************************
  * @file       mq2.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      MQ2 烟雾传感器换算
  *
  * @note       模块输出 AO 电压，经 ADC 采样后换算成 Rs，
  *             再用 Rs/R0 和曲线估算烟雾浓度。
  *             ppm 是估算值，不是实验室精度。
  ******************************************************************************
  */

#include "mq2/bsp_mq2.h"

/* 最近一次 MQ2 的 ADC 原始值和换算结果，供 Modbus/打印读取 */
volatile mq2_data mq2_sensor_data;

/**
  * @brief  把 MQ2 的 ADC 原始值换算成 ppm
  * @param  adc_value：ADC 原始值，范围 0~4095
  * @retval 估算 ppm；低于 300 返回 0，高于 10000 返回 10000
  * @note   VC_MQ2 是模块供电/分压前的满量程电压；
  *         当前模块自带分压，按 5V 还原 AO 电压。
  *         R0、RL 和曲线系数必须和实际模块匹配，否则 ppm 不准。
  */
float Get_Mq2_ppm(uint16_t adc_value)
{
    float vrl;       /* AO 还原后的模拟电压 */
    float Rs;        /* 当前传感器电阻，单位与 RL 一致 */
    float Mq2_ppm;   /* 估算的气体浓度 */

    /* 1. ADC 原始值 -> AO 电压
       当前代码按模块分压后的 5V 满量程还原 */
    vrl = (float)adc_value / 4095.0f * VC_MQ2;

    /* 2. 分压公式反推传感器电阻
       VRL = VCC * RL / (Rs + RL)
       Rs  = (VCC - VRL) * RL / VRL */
    Rs = (VC_MQ2 - vrl) * RL_MQ2 / vrl;

    /* 3. 用 datasheet 的 log-log 曲线估算 ppm
       y = a * x^b，x = Rs / R0 */
    Mq2_ppm = A_MQ2 * pow(Rs / R0_MQ2, B_MQ2);

    /* 4. 限幅：低于检测下限按 0，高于上限按 10000 */
    if (Mq2_ppm < 300)
        return 0.0f;
    else if (Mq2_ppm > 10000)
        return 10000.0f;
    else
        return Mq2_ppm;
}
