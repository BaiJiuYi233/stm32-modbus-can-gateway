/**
  ******************************************************************************
  * @file       pool.c
  * @author     baijiuyi233
  * @version    V1.0
  * @date       2026
  * @brief      内存池
  *
  ******************************************************************************
	*/
#include "malloc\pool.h"

typedef union pool_block 
{
	union pool_block* next;
	uint8_t           data[POOL_BLOCK_SIZE];
	uint32_t          align;      /* 占位，保证 4 字节对齐 */

}pool_block_t;

/* static 全局：启动时被 C 运行时清零（.bss）。
   g_free_list 指向第一个空闲块；
   块在链表里 = 空闲，不在链表里 = 已分配。
   没有单独的 used 标志；g_used 只是“已分配块数”的计数。 */
static pool_block_t  g_blocks[POOL_BLOCK_COUNT] = {0};
static pool_block_t* g_free_list = NULL;
static uint16_t      g_used = 0;


/* 初始化：把所有块串成一个空闲链表。
   注意：只在启动时调用一次；运行中调用会把已经分配出去的块
   也串回空闲链表，之后再 alloc 会拿到别人正在用的块。 */
void pool_init(void)
{
    g_free_list = &g_blocks[0]; // 空闲链表头 = 第 0 块
    g_used = 0; // 已分配块数清零

    for (uint16_t i = 0; i + 1 < POOL_BLOCK_COUNT; i++) {
        g_blocks[i].next = &g_blocks[i + 1]; // 当前块的下一个 = 下一块
    }

    g_blocks[POOL_BLOCK_COUNT - 1].next = NULL; // 最后一块的 next = NULL，链表结束
}

/* 分配：从链表头摘一块 */
void *pool_alloc(void)
{
    if (g_free_list == NULL) {
        return NULL;              /* 池子空了 */
    }

    pool_block_t *block = g_free_list; // 取出当前空闲块（不是“创建”）
    g_free_list = block->next; // 头移到下一块；该块不再在空闲链表里
    g_used++; // 已分配块数 +1

    return block->data;           // 返回数据区地址（data 在 offset 0）
}

/* 归还：把块挂回链表头 */
void pool_free(void *p)
{
    if (p == NULL) {
        return; 
    }

    pool_block_t *block = (pool_block_t *)p;  // 用户数据指针转回块指针（data 在 offset 0）
    block->next = g_free_list; // 当前块的下一个 = 旧链表头
    g_free_list = block; // 当前块成为新的空闲链表头

    if (g_used > 0) {
        g_used--; // 已分配块数 -1
    }
}

/* 分配并清零：比 pool_alloc 多一次 memset。
   注意：不要先 memset 再读 block->next，否则链表会断。 */
void *pool_alloc_zero(void)
{
    void *p = pool_alloc();
    if (p != NULL) {
        memset(p, 0, POOL_BLOCK_SIZE);
    }
    return p;
}

// 当前已分配块数
uint16_t pool_used(void)
{
    return g_used;
}

// 当前空闲块数
uint16_t pool_free_count(void)
{
    return (uint16_t)(POOL_BLOCK_COUNT - g_used);
}