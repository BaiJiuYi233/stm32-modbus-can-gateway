

/*==============================================================================
 *  FreeRTOSConfig.h —— FreeRTOS 总控面板
 *  1=开启该功能，0=关闭（不要删行，删了内核会以为你没配置）
 *============================================================================*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * 断言
 *-----------------------------------------------------------*/
#define configASSERT_DEFINED                             1       /* 1: 开启参数检查, 0: 关闭（关闭后传错参数直接HardFault） */
extern void vAssertCalled( void );                              /* 实现在 Userfreertos_hooks.c */
#define configASSERT( x )    if( ( x ) == 0 ) vAssertCalled()   /* 条件不成立就跳去 vAssertCalled() */
#define configQUEUE_REGISTRY_SIZE                        20      /* 最多注册20个队列/信号量供调试查看, 0: 关闭该功能 */


/*-----------------------------------------------------------
 * 调度行为
 *-----------------------------------------------------------*/
#define configUSE_PREEMPTION                             1       /* 1: 抢占式调度器, 0: 协程式调度器, 无默认需定义 */
#define configUSE_TIME_SLICING                           1       /* 1: 同优先级任务轮转, 0: 不轮转 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION          1       /* 1: 硬件指令选任务(最多32优先级), 0: 通用C循环 */


/*-----------------------------------------------------------
 * 钩子函数：下面填 1 的，必须自己写实现（已写在 Userfreertos_hooks.c）
 *-----------------------------------------------------------*/
#define configUSE_IDLE_HOOK                              1       /* 1: 空闲时调用vApplicationIdleHook(), 0: 不调用 */
#define configUSE_TICK_HOOK                              1       /* 1: 每1ms调用vApplicationTickHook(), 0: 不调用。中断里跑,禁耗时 */
#define configUSE_DAEMON_TASK_STARTUP_HOOK               0       /* 1: 要写守护任务启动钩子, 0: 不用 */

/* FreeRTOS中断服务函数相关定义 */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler

/*-----------------------------------------------------------
 * 时基
 *-----------------------------------------------------------*/
#define configCPU_CLOCK_HZ                               ( ( unsigned long ) 72000000 )   /* CPU主频, 必须等于真实主频72MHz */
#define configTICK_RATE_HZ                               ( ( TickType_t ) 100 )          /* 节拍频率, 1000=1ms一个节拍 */
#define configMINIMAL_STACK_SIZE                         ( ( unsigned short ) 128 )       /* 最小栈, 单位字(×4=512字节), 也是空闲任务栈 */
#define configTOTAL_HEAP_SIZE                            ( ( size_t ) ( 10*1024 ) )       /* 堆总大小(字节), 所有任务都从这里切 */
#define configMAX_TASK_NAME_LEN                          ( 16 )                           /* 任务名最长字符数(含结尾0), 即最多9个字 */
#define configUSE_TRACE_FACILITY                         1       /* 1: 可打印任务状态表调试, 0: 关闭省RAM */
#define configUSE_STATS_FORMATTING_FUNCTIONS             1       /* 1: 编出vTaskList/vTaskGetRunTimeStats, 0: 不编(上面开关就白开了) */
#define configUSE_16_BIT_TICKS                           0       /* 1: 节拍用16位(65秒后回绕), 0: 用32位, 必须保持0 */
#define configIDLE_SHOULD_YIELD                          1       /* 1: 空闲任务主动让CPU, 0: 不让 */
#define configUSE_CO_ROUTINES                            0       /* 1: 启用协程, 0: 不用（现代项目都用任务） */


/*-----------------------------------------------------------
 * 优先级与队列
 *-----------------------------------------------------------*/
#define configMAX_PRIORITIES                             ( 32 )                          /* 优先级等级数, 0~31, 0级留给空闲任务 */
#define configMAX_CO_ROUTINE_PRIORITIES                  ( 2 )                           /* 协程优先级数, 协程没开无影响 */
#define configTIMER_QUEUE_LENGTH                         20                              /* 定时器命令队列长度 */
#define configTIMER_TASK_PRIORITY                        ( configMAX_PRIORITIES - 1 )    /* 定时器任务优先级=31(最高) */
#define configUSE_COUNTING_SEMAPHORES                    1       /* 1: 支持计数信号量, 0: 不支持 */


/*-----------------------------------------------------------
 * 内存分配：MemMang 里只能加一个 heap_x.c，你用 heap_4.c
 *-----------------------------------------------------------*/
#define configSUPPORT_DYNAMIC_ALLOCATION                 1       /* 1: 支持动态分配(xTaskCreate), 0: 不支持 */
#define configSUPPORT_STATIC_ALLOCATION                  1       /* 1: 支持静态分配(要写两个GetMemory函数), 0: 不用写 */


/*-----------------------------------------------------------
 * 钩子与锁
 *-----------------------------------------------------------*/
#define configUSE_MALLOC_FAILED_HOOK                     1       /* 1: 堆不够时调用失败钩子, 0: 不调用(直接HardFault) */
#define configCHECK_FOR_STACK_OVERFLOW                   2       /* 2: 每次切换都查栈溢出(推荐), 0: 不查(溢出后静默踩坏堆块, 表现为随机HardFault) */
#define configUSE_MUTEXES                                1       /* 1: 支持互斥锁, 0: 不支持。Cmd和Turn抢电机要用 */
#define configUSE_RECURSIVE_MUTEXES                      1       /* 1: 支持递归互斥锁, 0: 不支持 */
#define configUSE_TIMERS                                 1       /* 1: 启用软件定时器(多一个任务+1KB栈), 0: 关闭省RAM */
#define configTIMER_TASK_STACK_DEPTH                     ( configMINIMAL_STACK_SIZE * 2 )  /* 定时器任务栈=256字=1KB */


/*-----------------------------------------------------------
 * API 裁剪：1=编进固件能用, 0=不编(调用会报未定义, 但省Flash)
 *-----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet                  1       /* 1: 能运行中改优先级, 0: 不能 */
#define INCLUDE_uxTaskPriorityGet                 1       /* 1: 能读任务优先级, 0: 不能 */
#define INCLUDE_vTaskDelete                       1       /* 1: 能删任务, 0: 不能 */
#define INCLUDE_vTaskCleanUpResources             0       /* 1: 删任务后清资源(很少用), 0: 不清 */
#define INCLUDE_vTaskSuspend                      1       /* 1: 能挂起/恢复任务, 0: 不能 */
#define INCLUDE_vTaskDelayUntil                   1       /* 1: 能用绝对周期延时, 0: 不能 */
#define INCLUDE_vTaskDelay                        1       /* 1: 能用相对延时(必须有), 0: 不能 */
#define INCLUDE_uxTaskGetStackHighWaterMark       1       /* 1: 能查栈剩余水位, 0: 不能 */
#define INCLUDE_uxTaskGetStackHighWaterMark2      1       /* 1: 同上(返回类型不同), 0: 不能 */
#define INCLUDE_xTaskGetSchedulerState            1       /* 1: 能查调度器状态, 0: 不能。SysTick_Handler用了 */
#define INCLUDE_xTimerGetTimerDaemonTaskHandle    1       /* 1: 能拿定时器任务句柄, 0: 不能 */
#define INCLUDE_xTaskGetIdleTaskHandle            1       /* 1: 能拿空闲任务句柄, 0: 不能 */
#define INCLUDE_xTaskGetHandle                    1       /* 1: 能按名字找任务, 0: 不能 */
#define INCLUDE_eTaskGetState                     1       /* 1: 能查任务状态, 0: 不能 */
#define INCLUDE_xSemaphoreGetMutexHolder          1       /* 1: 能查互斥锁被谁拿着, 0: 不能 */
#define INCLUDE_xTimerPendFunctionCall            1       /* 1: 能让定时器任务代跑函数, 0: 不能 */
#define INCLUDE_xTaskAbortDelay                   1       /* 1: 能强制打断任务延时, 0: 不能 */
#define INCLUDE_xTaskResumeFromISR                1       /* 1: 打开中断服务函数, 0: 不能 */ 



/* 配置抢占优先 */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

#define projCOVERAGE_TEST                               0       /* 1: 开覆盖率测试, 0: 关闭。你的工程不用 */

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15     /* 内核中断（SysTick 打节拍 / PendSV 换任务）*/
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5      /* 资格线的编号 = 5：中断里要调以 FromISR 结尾的 API，该中断的编号必须 >= 5。*/
#define configKERNEL_INTERRUPT_PRIORITY                 ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )   /* 换算后 = 240，芯片认这个 */         
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )  /* 换算后 = 80，芯片认这个 */          
/* 只有 Cortex-A（ARM_CA9）的移植用得到这个名字；Cortex-M3 用不到，留着无害 */
#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY

extern void vLoggingPrintf( const char * pcFormatString, ... );  /* 摆设：相关开关没开, 内核不会调用 */


#ifdef HEAP3
    /* heap_4, 这段不生效。 */
    #define xPortGetMinimumEverFreeHeapSize    ( x )
    #define xPortGetFreeHeapSize               ( x )
#endif

#endif /* FREERTOS_CONFIG_H */
