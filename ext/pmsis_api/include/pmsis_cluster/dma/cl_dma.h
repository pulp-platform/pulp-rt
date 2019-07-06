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

#ifndef __CL_DMA_H__
#define __CL_DMA_H__

#include "pmsis_cluster/cl_pmsis_types.h"
/**
 * @ingroup groupCluster
 */


/**
 * @defgroup DMA DMA management
 *
 * The cluster has its own local memory for fast access from the cluster cores
 * while the other memories are relatively slow if accessed by the cluster.
 * To keep all the cores available for computation each cluster contains a DMA
 * unit whose role is to asynchronously transfer data between a remote memory
 * and the cluster memory.
 *
 * The following API can be used to control the DMA in various ways:
 *   - Simple completion. This is only usable by a cluster. A set of transfers
 *   can be queued together. An identifier is allocated for the first transfer
 *   and reused for the following transfers. This identifier can then be used to
 *   block the calling core until all transfers are completed.
 *   - Event-based completion. This can be used on either the fabric controller
 *   or a cluster to enqueue a transfer and get notified on completion via an
 *   event on the fabric controller.
 */

/**
 * @addtogroup DMA
 * @{
 */

/**@{*/

/**
 * \brief DMA transfer direction.
 *
 * Describes the direction for a DMA transfer.
 */
/*!< Transfer from cluster memory to external memory. */
#define CL_DMA_DIR_LOC2EXT 0
/*!< Transfer from external memory to cluster memory. */
#define CL_DMA_DIR_EXT2LOC  1
typedef uint8_t cl_dma_dir_e;

#define CL_DMA_COMMON \
    uint32_t ext;\
    uint32_t loc;\
    uint32_t id;\
    uint16_t size;\
    cl_dma_dir_e dir;\
    uint8_t merge;


/** \brief DMA 1D copy structure.
 *
 * This structure is used by the runtime to manage a 1d DMA copy.
 * It must be instantiated once for each copy and must be kept alive
 * until the copy is finished.
 * It can be instantiated as a normal variable, for example as a global variable,
 * a local one on the stack,
 * or through the memory allocator.
 */
typedef struct cl_dma_copy_s
{
    CL_DMA_COMMON
    // 2d transfers args
    uint32_t stride;
    uint32_t length;
} cl_dma_copy_t;

/** \brief DMA 2D copy structure.
 *
 * This structure is used by the runtime to manage a 2d DMA copy.
 * It must be instantiated once for each copy and must be kept alive
 * until the copy is finished.
 * It can be instantiated as a normal variable, for example as a global variable,
 * a local one on the stack,
 * or through the memory allocator.
 */
typedef cl_dma_copy_t cl_dma_copy_2d_t;

/** \brief 1D DMA memory transfer.
 *
 * This enqueues a 1D DMA memory transfer (i.e. classic memory copy) with simple
 * completion based on transfer identifier.
 *
 * This can only be called on a cluster.
 *
 * \param   copy    The structure for the copy. This can be used with cl_dma_wait
 * to wait for the completion of this transfer.
 */
static inline void cl_dma_memcpy(cl_dma_copy_t *copy);

/** \brief 2D DMA memory transfer.
 *
 * This enqueues a 2D DMA memory transfer (rectangle area) with simple completion
 * based on transfer identifier.
 *
 * This can only be called on a cluster.
 * \param   copy    The structure for the copy. This can be used with
 * cl_dma_wait to wait for the completion of this transfer.
 */
static inline void cl_dma_memcpy_2d(cl_dma_copy_2d_t *copy);

/** \brief Simple DMA transfer completion flush.
 *
 * This blocks the core until the DMA does not have any pending transfers.
 *
 * This can only be called on a cluster.
 */
static inline void cl_dma_flush();

/** \brief Simple DMA transfer completion wait.
 *
 * This blocks the core until the specified transfer is finished. The transfer
 * must be described trough the identifier returned by the copy function.
 *
 * This can only be called on a cluster.
 *
 * \param   copy  The copy structure (1d or 2d).
 */
static inline void cl_dma_wait(void *copy);

typedef struct cl_dma_cmd_s cl_dma_cmd_t;

static inline void cl_dma_cmd(uint32_t ext, uint32_t loc, uint32_t size, cl_dma_dir_e dir, cl_dma_cmd_t *cmd);

static inline void cl_dma_cmd_2d(uint32_t ext, uint32_t loc, uint32_t size, uint32_t stride, uint32_t length, cl_dma_dir_e dir, cl_dma_cmd_t *cmd);

static inline void cl_dma_cmd_wait(cl_dma_cmd_t *cmd);

#ifndef PMSIS_NO_INLINE_INCLUDE

static inline void cl_dma_memcpy(cl_dma_copy_t *copy)
{
    if(!copy->merge)
    {// if copy is unique, give it an id
        copy->id = hal_read32(&DMAMCHAN->CMD);
    }
    hal_write32(&DMAMCHAN->CMD,copy->size
            | (copy->dir << DMAMCHAN_CMD_TYP_Pos)
            | (!copy->merge << (DMAMCHAN_CMD_INC_Pos))
            | (1 << 19)
            | (1 << 21)
            );
    hal_write32(&DMAMCHAN->CMD,copy->loc);
    hal_write32(&DMAMCHAN->CMD,copy->ext);
}

static inline void cl_dma_memcpy_2d(cl_dma_copy_t *copy)
{
    if(!copy->merge)
    {// if copy is unique, give it an id
        copy->id = hal_read32(&DMAMCHAN->CMD);
    }
    hal_write32(&DMAMCHAN->CMD,copy->size
            | (copy->dir << DMAMCHAN_CMD_TYP_Pos)
            | (!copy->merge << DMAMCHAN_CMD_INC_Pos)
            | (1 << DMAMCHAN_CMD_2D_Pos)); // 2d transfer
    hal_write32(&DMAMCHAN->CMD,copy->loc);
    hal_write32(&DMAMCHAN->CMD,copy->ext);
    // 2d part of the transfer
    hal_write32(&DMAMCHAN->CMD, copy->length
            | (copy->stride << DMAMCHAN_CMD_2D_STRIDE_Pos));
}

static inline void cl_dma_flush()
{
    while(hal_read32(&DMAMCHAN->STATUS));

    // Free all counters
    hal_write32(&DMAMCHAN->STATUS, -1);
}

static inline void cl_dma_wait(void *copy)
{
    cl_dma_copy_t *_copy = (cl_dma_copy_t *) copy;
    while((hal_read32(&DMAMCHAN->STATUS) >> _copy->id ) & 0x1 );

    hal_write32(&DMAMCHAN->STATUS, (0x1<<_copy->id));
}

struct cl_dma_cmd_s
{
  int id;
};

static inline void cl_dma_cmd(uint32_t ext, uint32_t loc, uint32_t size, cl_dma_dir_e dir, cl_dma_cmd_t *cmd)
{
  cl_dma_copy_t copy;
  copy.merge = 0;
  copy.ext = ext;
  copy.loc = loc;
  copy.size = size;
  copy.dir = dir;
  cl_dma_memcpy(&copy);
  cmd->id = copy.id;
}

static inline void cl_dma_cmd_2d(uint32_t ext, uint32_t loc, uint32_t size, uint32_t stride, uint32_t length, cl_dma_dir_e dir, cl_dma_cmd_t *cmd)
{
  cl_dma_copy_2d_t copy;
  copy.merge = 0;
  copy.ext = ext;
  copy.loc = loc;
  copy.size = size;
  copy.dir = dir;
  copy.length = length;
  copy.stride = stride;
  cl_dma_memcpy_2d(&copy);
  cmd->id = copy.id;
}

static inline void cl_dma_cmd_wait(cl_dma_cmd_t *cmd)
{
  cl_dma_copy_t copy;
  copy.id = cmd->id;
  cl_dma_wait(&copy);
}

#endif


#endif
