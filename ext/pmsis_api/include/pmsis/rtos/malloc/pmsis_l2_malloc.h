#ifndef __PMSIS_L2_MALLOC_H__
#define __PMSIS_L2_MALLOC_H__

#include "pmsis/pmsis_types.h"

#ifndef __L2_MALLOC_NATIVE__
void *pmsis_l2_malloc(int size);

void pmsis_l2_malloc_free(void *_chunk, int size);

void *pmsis_l2_malloc_align(int size, int align);

void pmsis_l2_malloc_init(void *heapstart, uint32_t size);

//void pmsis_l2_malloc_set_malloc_struct(malloc_t malloc_struct);

//malloc_t pmsis_l2_malloc_get_malloc_struct(void);
#endif
#endif
