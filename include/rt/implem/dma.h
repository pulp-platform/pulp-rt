/*
 * Copyright (C) 2018 ETH Zurich, University of Bologna and GreenWaves Technologies
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __RT_IMPLEM_DMA_H__
#define __RT_IMPLEM_DMA_H__

#include "pmsis_cluster/dma/cl_dma.h"

static inline void cl_dma_memcpy(cl_dma_copy_t *copy)
{
  rt_dma_memcpy(copy->ext, copy->loc, copy->size, copy->dir, copy->merge, (rt_dma_copy_t *)copy);
}


static inline void cl_dma_memcpy_2d(cl_dma_copy_2d_t *copy)
{
  rt_dma_memcpy_2d(copy->ext, copy->loc, copy->size, copy->stride, copy->length, copy->dir, copy->merge, (rt_dma_copy_t *)copy);
}


static inline void cl_dma_flush()
{
  rt_dma_flush();
}


static inline void cl_dma_wait(void *copy)
{
  rt_dma_wait(copy);
}

struct cl_dma_cmd_s
{
  int id;
};

static inline void cl_dma_cmd(uint32_t ext, uint32_t loc, uint32_t size, cl_dma_dir_e dir, cl_dma_cmd_t *cmd)
{
  rt_dma_memcpy(ext, loc, size, dir, 0, (rt_dma_copy_t *)cmd);
}

static inline void cl_dma_cmd_2d(uint32_t ext, uint32_t loc, uint32_t size, uint32_t stride, uint32_t length, cl_dma_dir_e dir, cl_dma_cmd_t *cmd)
{
  rt_dma_memcpy_2d(ext, loc, size, stride, length, dir, 0, (rt_dma_copy_t *)cmd);
}

static inline void cl_dma_cmd_wait(cl_dma_cmd_t *cmd)
{
  rt_dma_wait((rt_dma_copy_t *)cmd);
}

#endif