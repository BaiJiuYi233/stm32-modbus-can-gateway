/**
  ******************************************************************************
  * @file       bsp_temp.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      NTC 热敏电阻温度换算
  *
  * @note       ADC 读 PA4 的分压电压，反推 NTC 电阻，
  *             再用 B 值公式换算成摄氏度。
  ******************************************************************************
  */

#include "temp/bsp_temp.h"
#include "adc/bsp_adc.h"

/* 最近一次 NTC 的 ADC 原始值和温度，供 Modbus/打印读取 */
volatile tmpe_data g_sensor_data;

/**
  * @brief  把 NTC 的 ADC 原始值换算成摄氏度
  * @param  adc_value：ADC 原始值，范围 0~4095
  * @retval 摄氏度
  * @note   假设 NTC 接 VCC、固定电阻接 GND，且 ADC 参考为 3.3V。
  *         如果温度变化方向相反，交换分压公式的分子/分母。
  */
float Calculate_Temperature(uint16_t adc_value)
{
    float voltage = 0.0f;
    float r_ntc = 0.0f;
    float temp_kelvin = 0.0f;
    float temp_celsius = 0.0f;

    /* 1. 防呆：避免 ADC 为 0 或满量程时除以 0/对数溢出 */
    if (adc_value == 0) adc_value = 1;
    if (adc_value >= 4095) adc_value = 4094;

    /* 2. ADC 原始值 -> 引脚电压（ADC 参考 3.3V） */
    voltage = ((float)adc_value / 4095.0f) * 3.3f;

    /* 3. 分压反推 NTC 电阻
       NTC 接 VCC、固定电阻接 GND：
       V = 3.3 * Rfixed / (Rntc + Rfixed)
       Rntc = Rfixed * V / (3.3 - V) */
    r_ntc = FIXED_RESISTOR * (voltage / (3.3f - voltage));

    /* 4. B 值公式：
       1/T = 1/T0 + (1/B) * ln(R/R0) */
    temp_kelvin = 1.0f / ((1.0f / 298.15f) +
                          (1.0f / NTC_B_VALUE) * log(r_ntc / NTC_R25));

    /* 5. 开尔文 -> 摄氏度 */
    temp_celsius = temp_kelvin - 273.15f;

    return temp_celsius;
}
