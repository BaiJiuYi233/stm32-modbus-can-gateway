/**
  ******************************************************************************
  * @file       freertos_hooks.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief     RTOS失败钩子配置和静态空闲任务分配
  *
  ******************************************************************************
	*/


#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/* ------- 1. 断言失败 ------- */
/* configASSERT 里失败会跳到这里；调试时在这里下断点，能直接看到是哪个 API 参数非法 */
void vAssertCalled(void)
{
    taskDISABLE_INTERRUPTS();
    __asm volatile ("bkpt 0");     /* 挂上调试器时停在这；不接调试器会走 while(1) */
    while (1);
}

/* ------- 2. 空闲任务的内存（静态分配模式必须提供） ------- */
static StaticTask_t xIdleTaskTCB;
static StackType_t  ucIdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = ucIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

/* ------- 3. 定时器服务任务的内存（开了 configUSE_TIMERS 才需要） ------- */
static StaticTask_t xTimerTaskTCB;
static StackType_t  ucTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    uint32_t      *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = ucTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ------- 4. 空闲钩子：空闲任务每轮跑一次 ------- */
void vApplicationIdleHook(void)
{
    /* 什么都不做也可以。以后想加 IWDG 喂狗就放这 */
}

/* ------- 5. 节拍钩子：每个 1ms 节拍跑一次，在中断上下文里 ------- */
void vApplicationTickHook(void)
{
    /* 严禁 HAL_Delay / printf / 任何阻塞操作，这里只有 1ms 预算 */
}

/* ------- 6. 堆分配失败：比 HardFault 友好一百倍 ------- */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    __asm volatile ("bkpt 0");
    while (1);                     /* 停在这，说明 configTOTAL_HEAP_SIZE 给小了 */
}

/* ------- 7. 栈溢出钩子：开了 configCHECK_FOR_STACK_OVERFLOW 就必须提供实现 ------- */
/* 任务栈写穿了会先跳到这里，而不是等下一次 malloc 才 HardFault。
   停在这的时候，看 pcTaskName 就知道是哪个任务的栈给小了。 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    __asm volatile ("bkpt 0");
    while (1);
}