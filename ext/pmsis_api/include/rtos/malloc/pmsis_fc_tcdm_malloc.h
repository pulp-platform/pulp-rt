#ifndef __PMSIS_FC_TCDM_MALLOC_H__
#define __PMSIS_FC_TCDM_MALLOC_H__

#include "pmsis_hal.h"
#include "pmsis_malloc_internal.h"

void *pmsis_fc_tcdm_malloc(uint32_t size);

void pmsis_fc_tcdm_malloc_free(void *_chunk, int size);

void *pmsis_fc_tcdm_malloc_align(int size, int align);

void pmsis_fc_tcdm_malloc_init(void *heapstart, uint32_t size);

void pmsis_fc_tcdm_malloc_set_malloc_struct(malloc_t malloc_struct);

malloc_t pmsis_fc_tcdm_malloc_get_malloc_struct(void);

#endif
