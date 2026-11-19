# STM32F103 + FreeRTOS 多协议分布式网关

一个基于 3 块 **STM32F103C8T6** 的分布式网关学习项目：

- **A 板**：Modbus RTU 主站 + CAN 接收 + 数据打印
- **B 板**：Modbus RTU 从站 + 温度/MQ135/MQ2 采集
- **C 板**：CAN 报警节点 + 火焰检测 + 蜂鸣器

目标不是做一个“能跑的 demo”，而是把 **RS485/Modbus、CAN、FreeRTOS 任务、队列、传感器采集、协议解析** 串成一条完整链路。

> 说明：火焰报警部分仅用于学习和演示，不是消防安全设备。

---

## 1. 系统架构

```text
                 ┌──────────────────────────┐
                 │        A 板（网关）       │
                 │  Modbus RTU Master       │
                 │  CAN Receiver            │
                 │  USART1 Debug            │
                 └───────┬──────────┬───────┘
                         │          │
                  RS485  │          │ CAN
                         │          │
                 ┌───────▼───┐  ┌───▼────────┐
                 │  B 板      │  │  C 板       │
                 │ Modbus     │  │ CAN Node    │
                 │ Slave 2    │  │ Flame       │
                 │ NTC        │  │ Buzzer      │
                 │ MQ135      │  │             │
                 │ MQ2        │  │             │
                 └────────────┘  └─────────────┘
```

### 数据流

```text
B：NTC/MQ135/MQ2 -> Sensor_task -> Modbus 寄存器
A：RS485 请求 -> B 从站回复 -> A 解析 -> 串口打印

C：火焰传感器 -> CAN 0x180 报警帧
A：CAN 接收中断 -> 队列 -> CAN_RX_Task -> 串口打印
```

---

## 2. 硬件

| 板子 | 主要硬件 | 作用 |
|---|---|---|
| A | STM32F103C8T6、RS485 模块、CAN 模块 | 网关/主站 |
| B | STM32F103C8T6、RS485 模块、NTC、MQ135、MQ2 | 环境采集从站 |
| C | STM32F103C8T6、CAN 模块、火焰传感器、蜂鸣器 | 报警节点 |
| 调试 | USB-TTL | 查看 A/B/C 的串口日志 |

### 引脚分配

#### A 板

| 功能 | 引脚 |
|---|---|
| 调试串口 USART1 | PA9 / PA10 |
| RS485 USART3 | PB10 / PB11 |
| RS485 DIR | PB8 |
| CAN | PA11 / PA12 |
| LED | PA1 / PA2 / PA3 |

#### B 板

| 功能 | 引脚 |
|---|---|
| 调试串口 USART1 | PA9 / PA10 |
| RS485 USART3 | PB10 / PB11 |
| RS485 DIR | PB8 |
| NTC | PA4 / ADC1_IN4 |
| MQ135 | PA5 / ADC1_IN5 |
| MQ2 | PA6 / ADC1_IN6 |

#### C 板

| 功能 | 引脚 |
|---|---|
| 调试串口 USART1 | PA9 / PA10 |
| CAN | PA11 / PA12 |
| 火焰传感器 DO | PA5 |
| 蜂鸣器 | PA4 |

> MQ2/MQ135 一般需要 5V 加热。  
> 如果 AO 输出可能超过 3.3V，必须确认模块自带分压，或外部加分压；不要直接把 5V 送进 STM32 ADC。

---

## 3. 当前已实现的功能

### A 板

- Modbus RTU 主站，RS485 9600 8N1；
- 每秒请求 B 的 4 个保持寄存器：
  - 温度 ×100
  - 采样序号
  - MQ135
  - MQ2
- 校验地址、CRC、功能码、字节数；
- 解析成传感器表并逐行打印；
- CAN 接收 `0x180` 报警帧；
- CAN 中断入队，任务中打印，不在中断里 `printf`。

### B 板

- Modbus RTU 从站，地址 2；
- 只实现 `0x03` 读保持寄存器；
- 一个 `Sensor_task` 顺序采集：
  - NTC 温度
  - MQ135
  - MQ2
- 每 500ms 更新一次；
- 通过队列把 Modbus 请求从 ISR 交给 `Modbus_Task`；
- 半双工 RS485 的 DIR/TC 处理。

### C 板

- 每 1 秒发送标准 CAN 帧 `0x180`；
- 数据格式：

  ```text
  data[0] = 0x03        C 节点
  data[1] = 0x01        报警类型
  data[2] = seq         帧序号
  data[3] = flame_flags 火焰标志
  data[4..7] = 0
  ```

- 火焰传感器 DO 检测；
- 火焰触发时蜂鸣器响；
- 50ms 轮询一次。

---

## 4. Modbus 寄存器映射

B 板从站地址：`2`  
波特率：`9600 8N1`

| 寄存器 | 内容 | 说明 |
|---:|---|---|
| 0 | 温度 | int16，×100；25.60°C = 2560 |
| 1 | 采样序号 | 每次 Sensor_task 更新 +1 |
| 2 | MQ135 | ppm，uint16 |
| 3 | MQ2 | ppm，uint16 |
| 4 | 预留 | 0 |
| 5 | 预留 | 0 |
| 6 | 预留 | 0 |

A 的请求：

```text
02 03 00 00 00 04 CRC_L CRC_H
```

含义：

```text
2 号从站，用 0x03，从 0 号寄存器开始，读 4 个。
```

B 的响应：

```text
02 03 08 [温度高][温度低] [序号高][序号低]
         [MQ135高][MQ135低] [MQ2高][MQ2低] CRC_L CRC_H
```

> MQ 的 ppm 是估算值，不是实验室精度；模块的 R0/RL 和分压比会影响结果。  
> 当前更适合做趋势/阈值报警，不适合宣称“精确 ppm 检测”。

---

## 5. CAN 报警帧

标准帧 ID：`0x180`  
DLC：`8`

```text
Byte 0：0x03        C 节点
Byte 1：0x01        报警类型
Byte 2：seq         帧序号
Byte 3：flags       火焰标志
Byte 4~7：0
```

A 的 CAN 滤波器只接收 `0x180`。  
C 每 1 秒发送一帧，A 收到后打印。

> CAN 是广播/消息机制，不是电源开关。  
> “报警后断电”需要额外的继电器/MOSFET 执行器，不能靠 CAN 线直接断电。

---

## 6. 软件结构

```text
Project/
  Gateway_A.uvprojx        Keil 工程（A/B/C 三个 target）
User/
  main.c                   HAL 初始化、时钟、启动
  board/                   板型选择、外设初始化
  freertos_demo/           FreeRTOS 任务创建
  rs485/                   USART3、DIR、发送、接收回调
  modbus/                  Modbus 主站/从站、CRC、队列、寄存器
  adc/                     ADC1 单通道轮询
  temp/                    NTC 温度换算
  mq2/                     MQ2 换算
  mq135/                   MQ135 换算
  can/                     CAN 初始化、发送、接收、队列
  flame/                   火焰传感器 GPIO
  buzz/                    蜂鸣器
  led/ usart/ dwt/         基础 BSP
  malloc/pool.c            固定大小内存池示例
```

### FreeRTOS 任务（按板型裁剪）

| 任务 | 板型 | 作用 |
|---|---|---|
| `rs485_tx_task` | A | Modbus 主站轮询 |
| `CAN_RX_Task` | A | 从 CAN 队列取报警帧并打印 |
| `Sensor_task` | B | 采集 NTC/MQ135/MQ2 |
| `Modbus_Task` | B | 从 Modbus 队列取请求并回复 |
| `can_tx_task` | C | 周期性发送 `0x180` |
| `task4` | C | 火焰检测 + 蜂鸣器 |

---

## 7. 编译与烧录

### 环境

- Keil MDK 5；
- ARM Compiler 5（工程 `uAC6 = 0`）；
- STM32F1xx HAL；
- FreeRTOS。

### 三个 Target

打开：

```text
Project/Gateway_A.uvprojx
```

选择 target：

| Target | 板型宏 | 烧录到 |
|---|---|---|
| `A_Master` | `BOARD_A` | A 板 |
| `B_RS485` | `BOARD_B` | B 板 |
| `C_CAN` | `BOARD_C` | C 板 |

每个 target 的输出目录已经分开：

```text
.\Objects\A\
.\Objects\B\
.\Objects\C\
```

> 切换 target 后请 **Rebuild**，不要只点 Build。  
> 否则可能用到上一个 target 的 `.o`，出现莫名其妙的链接错误。

### Include Path

```text
..\User
```

### 串口参数

| 串口 | 用途 | 参数 |
|---|---|---|
| USART1 | 调试打印 | 115200 8N1 |
| USART3 | RS485/Modbus | 9600 8N1 |

---

## 8. 接线

### RS485（A ↔ B）

```text
A RS485 A  ─────  B RS485 A
A RS485 B  ─────  B RS485 B
A GND      ─────  B GND
```

- A/B 不要接反；
- 共地；
- 长线/多节点时加 120Ω 终端电阻；
- DIR 接 PB8。

### CAN（A ↔ C）

```text
A CANH ───── C CANH
A CANL ───── C CANL
A GND  ───── C GND
```

- 两端各 120Ω；
- CANH/CANL 不要接反；
- 共地。

### 调试

```text
USB-TTL RX -> 板子 PA9（USART1_TX）
USB-TTL TX -> 板子 PA10（USART1_RX）
USB-TTL GND -> GND
```

---

## 9. 预期日志

### A 板

```text
Modbus master: poll B addr=2
B: 温度:24.35C[正常]
B: 空气:123.00ppm[正常]
B: 烟雾:456.00ppm[正常]
计数：12

03 01 05 00 00 00 00 00
火焰传感器触发和蜂鸣器
```

### B 板

```text
Modbus RTU slave: addr=2, regs=7
```

### C 板

每次发送 CAN 帧时，`CAN_Send()` 会打印：

```text
CAN 发送数据成功!
发送的报文信息如下：
标准 ID：0x180
帧类型：数据帧
```

火焰触发时，C 的蜂鸣器响，A 打印：

```text
火焰传感器触发和蜂鸣器
```

---

## 10. 当前限制

- MQ2/MQ135 的 ppm 没有做严格标定，受 R0、RL、分压比、预热时间影响；
- 火焰报警是学习演示，不是消防设备；
- A 目前只通过调试串口打印，没有 PC 上位机/ESP8266/W25Q64；
- Modbus 只实现 `0x03` 读保持寄存器，没有写寄存器；
- A 的传感器表是本地表，新增传感器仍需同步改寄存器映射、请求数量和解析下标；
- A 的温度表当前使用绝对值，负温度显示可能不正确；
- C 的报警断电执行器尚未加入。

---

## 11. 后续计划

- [ ] 给 B 的 Modbus 增加状态位/故障位；
- [ ] 增加 PC 上位机曲线；
- [ ] 接入 ESP8266/WiFi 或 W25Q64 离线日志；
- [ ] 给 C 增加更多报警传感器；
- [ ] 整理最终演示视频和接线图。
