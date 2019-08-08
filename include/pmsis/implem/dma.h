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

#ifndef __PMSIS_IMPLEM_DMA_H__
#define __PMSIS_IMPLEM_DMA_H__

#include "pmsis/cluster/dma/cl_dma.h"
#include "pmsis.h"
#include "hal/pulp.h"

struct cl_dma_cmd_s
{
  int id;
};

static inline void __cl_dma_memcpy(unsigned int ext, unsigned int loc, unsigned short size, cl_dma_dir_e dir, int merge, cl_dma_cmd_t *copy)
{
#ifdef __RT_USE_PROFILE
  int trace = __rt_pe_trace[rt_core_id()];
  gv_vcd_dump_trace(trace, 4);
#endif

  int id = -1;
  if (!merge) id = plp_dma_counter_alloc();
  unsigned int cmd = plp_dma_getCmd(dir, size, PLP_DMA_1D, PLP_DMA_TRIG_EVT, PLP_DMA_NO_TRIG_IRQ, PLP_DMA_SHARED);
  // Prevent the compiler from pushing the transfer before all previous
  // stores are done
  __asm__ __volatile__ ("" : : : "memory");
  plp_dma_cmd_push(cmd, loc, ext);
  if (!merge) copy->id = id;

#ifdef __RT_USE_PROFILE
  gv_vcd_dump_trace(trace, 1);
#endif
}


static inline void __cl_dma_memcpy_2d(unsigned int ext, unsigned int loc, unsigned short size, unsigned short stride, unsigned short length, cl_dma_dir_e dir, int merge, cl_dma_cmd_t *copy)
{
#ifdef __RT_USE_PROFILE
  int trace = __rt_pe_trace[rt_core_id()];
  gv_vcd_dump_trace(trace, 4);
#endif

  int id = -1;
  if (!merge) id = plp_dma_counter_alloc();
  unsigned int cmd = plp_dma_getCmd(dir, size, PLP_DMA_2D, PLP_DMA_TRIG_EVT, PLP_DMA_NO_TRIG_IRQ, PLP_DMA_SHARED);
  // Prevent the compiler from pushing the transfer before all previous
  // stores are done
  __asm__ __volatile__ ("" : : : "memory");
  plp_dma_cmd_push_2d(cmd, loc, ext, stride, length);
  if (!merge) copy->id = id;

#ifdef __RT_USE_PROFILE
  gv_vcd_dump_trace(trace, 1);
#endif
}

static inline void __cl_dma_flush()
{
#ifdef __RT_USE_PROFILE
  int trace = __rt_pe_trace[rt_core_id()];
  gv_vcd_dump_trace(trace, 5);
#endif
  plp_dma_barrier();
#ifdef __RT_USE_PROFILE
  gv_vcd_dump_trace(trace, 1);
#endif
}

static inline void __cl_dma_wait(cl_dma_cmd_t *copy)
{
#ifdef __RT_USE_PROFILE
  int trace = __rt_pe_trace[rt_core_id()];
  gv_vcd_dump_trace(trace, 5);
#endif
  plp_dma_wait(copy->id);
#ifdef __RT_USE_PROFILE
  gv_vcd_dump_trace(trace, 1);
#endif
}

static inline void cl_dma_memcpy(cl_dma_copy_t *copy)
{
  __cl_dma_memcpy(copy->ext, copy->loc, copy->size, copy->dir, copy->merge, (cl_dma_cmd_t *)copy);
}


static inline void cl_dma_memcpy_2d(cl_dma_copy_2d_t *copy)
{
  __cl_dma_memcpy_2d(copy->ext, copy->loc, copy->size, copy->stride, copy->length, copy->dir, copy->merge, (cl_dma_cmd_t *)copy);
}


static inline void cl_dma_flush()
{
  __cl_dma_flush();
}


static inline void cl_dma_wait(void *copy)
{
  __cl_dma_wait(copy);
}

static inline void cl_dma_cmd(uint32_t ext, uint32_t loc, uint32_t size, cl_dma_dir_e dir, cl_dma_cmd_t *cmd)
{
  __cl_dma_memcpy(ext, loc, size, dir, 0, (cl_dma_cmd_t *)cmd);
}

static inline void cl_dma_cmd_2d(uint32_t ext, uint32_t loc, uint32_t size, uint32_t stride, uint32_t length, cl_dma_dir_e dir, cl_dma_cmd_t *cmd)
{
  __cl_dma_memcpy_2d(ext, loc, size, stride, length, dir, 0, (cl_dma_cmd_t *)cmd);
}

static inline void cl_dma_cmd_wait(cl_dma_cmd_t *cmd)
{
  __cl_dma_wait((cl_dma_cmd_t *)cmd);
}

#endif