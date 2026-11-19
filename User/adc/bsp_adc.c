/**
  ******************************************************************************
  * @file       bsp_adc.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      ADC1 初始化与单通道采样
  *
  * @note       本文件只负责“把模拟电压变成 0~4095 的原始值”。
  *             温度、烟雾、空气质量的具体换算放在传感器任务里做，
  *             不要在这里塞浮点公式或滤波。
  ******************************************************************************
  */

#include "adc/bsp_adc.h"

ADC_HandleTypeDef hadc1;   // ADC1 句柄；MspInit、采样函数都通过它访问 ADC1

/**
  * @brief  初始化 ADC1 的公共参数，并配置 PA4 为默认规则通道
  * @note   当前使用“单通道轮询”：
  *         adc_read_channel() 每次自己选通道、Rank1、启动、等待、停止。
  *         所以这里不启用扫描，也不配置第二个 Rank；
  *         以后要用 DMA 多通道时，再改成“扫描 + DMA”，不要和单通道轮询混用。
  * @param  无
  * @retval 无
  */
void ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  // 1. ADC1 公共参数
  hadc1.Instance = ADC1;                            // 使用 ADC1 外设
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;       // 单通道：不扫描
  hadc1.Init.ContinuousConvMode = DISABLE;          // 不连续转换：每次 Start 只转一次
  hadc1.Init.DiscontinuousConvMode = DISABLE;       // 不启用间断模式
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START; // 软件触发：HAL_ADC_Start() 才开始
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;       // 12 位结果右对齐：0~4095
  hadc1.Init.NbrOfConversion = 1;                   // 规则序列只有 1 个 Rank

  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();                                // 初始化失败，停在 Error_Handler
  }

  // 2. 默认通道：PA4 / ADC_CHANNEL_4，接热敏电阻
  //    adc_read_channel() 每次会重新配置成它要读的通道
  sConfig.Channel = ADC_CHANNEL_4;                  // PA4
  sConfig.Rank = ADC_REGULAR_RANK_1;                // 第 1 个转换
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5; // 高阻信号用长采样
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  ADC1 底层初始化（由 HAL_ADC_Init 自动调用）
  * @note   这里只开时钟、配 GPIO；当前用轮询，不在这里开 ADC 中断。
  * @param  adcHandle：ADC 句柄
  * @retval 无
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (adcHandle->Instance == ADC1)
  {
    // 1. 开 ADC1 时钟
    __HAL_RCC_ADC1_CLK_ENABLE();

    // 2. 开 GPIOA 时钟（PA4、PA5、PA6 都在 GPIOA）
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 3. PA4/PA5 设为模拟输入
    //    模拟模式会断开数字输入/输出，避免数字电路影响 ADC 读数
    GPIO_InitStruct.Pin = TEMP_Pin | MQ135_Pin | MQ2_Pin;   // PA4 + PA5
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;      // 模拟输入
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 注意：F103 的 ADC 时钟来自 PCLK2 再分频，最高 14MHz。
       当前工程没有在这里设置 ADCPRE；如果 ADC 读数异常，
       先查 RCC 的 ADC 预分频，常见配置是 72MHz / 6 = 12MHz。 */
  }
}

/**
  * @brief  ADC1 反初始化：关时钟、释放 GPIO
  * @param  adcHandle：ADC 句柄
  * @retval 无
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
  if (adcHandle->Instance == ADC1)
  {
    // 1. 关闭 ADC1 时钟
    __HAL_RCC_ADC1_CLK_DISABLE();

    // 2. 释放 PA4/PA5，恢复成复位后的状态
    HAL_GPIO_DeInit(GPIOA, TEMP_Pin | MQ135_Pin | MQ2_Pin);
  }
}

/**
  * @brief  读取一个 ADC 通道的原始值
  * @note   每次调用都会把 ADC 重配成“单通道、Rank1”，所以：
  *         - 不能在 DMA 多通道扫描正在跑的时候调用；
  *         - 只在传感器任务里调用，不要在多个任务里同时调用；
  *         - 读的是原始值 0~4095，换算成电压/温度在调用者里做。
  * @param  channel：ADC_CHANNEL_4 / ADC_CHANNEL_5 / ADC_CHANNEL_6 ...
  * @retval 0~4095 的原始值；出错也返回 0
  * @note   0 也可能是有效读数，调试时如果怀疑出错，可以再加错误码/超时标志。
  */
uint16_t adc_read_channel(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  // 1. 选通道、放到 Rank1、采样时间拉长
  //    高阻传感器（NTC 分压、MQ 模块）用 239.5 周期更稳
  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    return 0;
  }

  // 2. 软件启动一次转换
  HAL_ADC_Start(&hadc1);

  // 3. 等转换完成，最多等 10ms
  //    HAL_ADC_PollForConversion 的超时单位是毫秒
  if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);   // 超时要把 ADC 停掉，避免状态卡住
    return 0;
  }

  // 4. 读数据寄存器，然后停止 ADC
  uint16_t value = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  return value;
}
