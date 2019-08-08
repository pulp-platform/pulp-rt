#ifndef __PMSIS_L1_MALLOC_H__
#define __PMSIS_L1_MALLOC_H__

#include "pmsis_malloc_internal.h"

void *pmsis_l1_malloc(uint32_t size);

void pmsis_l1_malloc_free(void *_chunk, int size);

void *pmsis_l1_malloc_align(int size, int align);

void pmsis_l1_malloc_init(void *heapstart, uint32_t size);

void pmsis_l1_malloc_set_malloc_struct(malloc_t malloc_struct);

malloc_t pmsis_l1_malloc_get_malloc_struct(void);

#endif
