#ifndef __POOL_H
#define __POOL_H

#include "main.h"


#define POOL_BLOCK_SIZE    32

#define POOL_BLOCK_COUNT    8

#if (POOL_BLOCK_SIZE < 8)
#error "POOL_BLOCK_SIZE must be at least 8"
#endif

#if (POOL_BLOCK_COUNT < 1)
#error "POOL_BLOCK_COUNT must be at least 1"
#endif

void      pool_init(void);
void     *pool_alloc(void);
void      pool_free(void *p);
void 		 *pool_alloc_zero(void);
uint16_t  pool_used(void);
uint16_t  pool_free_count(void);

#endif /*__POOL_H */