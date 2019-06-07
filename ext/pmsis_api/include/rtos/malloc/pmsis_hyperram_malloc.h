#ifndef __PMSIS_HYPERRAM_MALLOC_H__
#define __PMSIS_HYPERRAM_MALLOC_H__

#include "pmsis.h"
#include "pmsis_malloc_internal.h"

void *pmsis_hyperram_malloc(int32_t size);

void pmsis_hyperram_malloc_free(void *_chunk, int32_t size);

void pmsis_hyperram_malloc_init(void *heapstart, int32_t size);

void pmsis_hyperram_malloc_deinit();

/*
void *pmsis_hyperram_malloc_align(int size, int align);

void pmsis_hyperram_malloc_set_malloc_struct(malloc_t malloc_struct);

malloc_t pmsis_hyperram_malloc_get_malloc_struct(void);
*/

#endif  /* __PMSIS_HYPERRAM_MALLOC_H__ */
