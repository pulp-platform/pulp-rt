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

/*
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "pmsis.h"


// If not NULL, this task is enqueued when the current transfer is finished.
RT_FC_TINY_DATA struct pi_fc_task *__rt_hyper_end_task;

// Following variables are used to reenqueue transfers to overcome burst limit.
// This is used directly by assebly to quickly reenqueue the transfer.
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_base;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_hyper_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_repeat;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_repeat_size;

// Head and tail of the queue of pending transfers which were put on hold
// as a transfer was already on-going.
RT_FC_TINY_DATA struct pi_fc_task *__rt_hyper_pending_tasks;
RT_FC_TINY_DATA struct pi_fc_task *__rt_hyper_pending_tasks_last;

// All the following are used to keep track of the current transfer when it is
// emulated due to aligment constraints.
// The interrupt handler executed at end of transfer will execute the FSM to reenqueue
// a partial transfer.
RT_FC_TINY_DATA int __rt_hyper_pending_emu_channel;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_hyper_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_size;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_size_2d;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_length;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_stride;
RT_FC_TINY_DATA unsigned char __rt_hyper_pending_emu_do_memcpy;
RT_FC_TINY_DATA struct pi_fc_task *__rt_hyper_pending_emu_task;

// Local task used to enqueue cluster requests.
// We cannot reuse the task coming from cluster side as it is used by the emulation
// state machine so we copy the request here to improve performance.
static struct pi_fc_task __pi_hyper_cluster_task;
static pi_cl_hyperram_req_t *__pi_hyper_cluster_reqs_first;
static pi_cl_hyperram_req_t *__pi_hyper_cluster_reqs_last;


// Hyper structure allocated when opening the driver
typedef struct {
  rt_extern_alloc_t *alloc;
  int channel;
  int alloc_init;
} pi_hyperram_t;


// UDMA hyper channel callbacks called when an hyper udma transfer is done.
extern void __rt_hyper_handle_copy();



// Allocate all resources for hyper driver, especially takes care of the hyperram allocator
static pi_hyperram_t *__pi_hyperram_init(int ramsize);

// Free all resources allocated for the driver
static void __pi_hyperram_free(pi_hyperram_t *hyper);

// Performs a direct aligned copy:
//  - hyper addr is multiple of 2
//  - l2 addr is multiple of 4
//  - size is multiple of 4
static void __attribute__((noinline)) __pi_hyper_copy_aligned(int channel,
  uint32_t addr, uint32_t _hyper_addr, uint32_t size, rt_event_t *event);

// Performs a misaligned 2d read without any constraint.
// This function can be either called directly or as an event callback
// This function is like a state machine,
// it checks the state of the pending copy and does one more step
// so that the whole transfer can be done asynchronously without blocking
// the core.
static int __pi_hyper_resume_misaligned_read(struct pi_fc_task *task);

// Performs a misaligned 2d write without any constraint.
// This function can be either called directly or as an event callback
// This function is like a state machine,
// it checks the state of the pending copy and does one more step
// so that the whole transfer can be done asynchronously without blocking
// the core.
static int __pi_hyper_resume_misaligned_write(struct pi_fc_task *task);

// Continue the pending misaligned transfer until nothing else can be done.
// This can also switch to the next pending task if the pending one is done.
static void __pi_hyper_copy_misaligned(struct pi_fc_task *task);

// Execute a 1D copy.
// Figure out if the copy can be pushed directly (if it has good alignments)
// or if it should be handled with partial copies
static void __pi_hyper_copy_exec(int channel, uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event);

// Execute a 2D copy.
// Contrary to 1D copies, 2D copies are always handled with partial copies
static void __pi_hyper_2d_copy_exec(int channel, uint32_t addr, uint32_t hyper_addr, uint32_t size, int stride, uint32_t length, rt_event_t *event);

// CHeck if there is a task waiting for execute it and if so, remove it from the queue
// and execute it
static void exec_pending_task();

// Try to trigger a copy. If there is already one pending, the copy is put on hold,
// otherwise it is execute.
static void __pi_hyper_copy(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event, int mbr);

// Try to trigger a 2d copy. If there is already one pending, the copy is put on hold,
// otherwise it is execute.
static void __pi_hyper_copy_2d(int channel, uint32_t addr, uint32_t hyper_addr, uint32_t size, int stride, int length, rt_event_t *event, int mbr);

// This is called by the interrupt handler when a transfer is finished and a pending
// misaligned transfer is detected, to continue it.
void __rt_hyper_resume_emu_task();

// This is called by the interrupt handler when a transfer is finished and a waiting
// transfer is detected, to execute it.
void __rt_hyper_resume_copy(struct pi_fc_task *task);

// Execute a transfer request from cluster side
static void __pi_hyperram_cluster_req_exec(pi_cl_hyperram_req_t *req);

// Handle end of cluster request, by sending the reply to the cluster
static void __pi_hyperram_cluster_req_done(void *_req);

// Handle a transfer request from cluster side.
// This will either execute it if none is pending or put it on hold
static void __pi_hyperram_cluster_req(void *_req);




#define __PI_HYPER_TEMP_BUFFER_SIZE 128

// Temporary buffer of size __PI_HYPER_TEMP_BUFFER_SIZE used for misaligned
// transfers between hyperram and L2
static char *__pi_hyper_temp_buffer;




void pi_hyperram_conf_init(struct pi_hyperram_conf *conf)
{
  conf->id = -1;
  conf->ram_size = 0;
}



int pi_hyperram_open(struct pi_device *device)
{
  struct pi_hyperram_conf *conf = (struct pi_hyperram_conf *)device->config;
  pi_hyperram_t *hyper = NULL;
  int periph_id;
  int channel;
  int ramsize;

  periph_id = ARCHI_UDMA_HYPER_ID(conf->id);
  channel = UDMA_EVENT_ID(periph_id);
  ramsize = conf->ram_size;

  hyper = __pi_hyperram_init(ramsize);
  if (hyper == NULL) goto error;

  hyper->channel = periph_id;

  // Activate routing of UDMA hyper soc events to FC to trigger interrupts
  soc_eu_fcEventMask_setEvent(channel);
  soc_eu_fcEventMask_setEvent(channel+1);

  // Deactivate Hyper clock-gating
  plp_udma_cg_set(plp_udma_cg_get() | (1<<periph_id));

  // Redirect all UDMA hyper events to our callback
  __rt_periph_channel(channel+1)->callback = __rt_hyper_handle_copy;
  __rt_periph_channel(channel)->callback = __rt_hyper_handle_copy;

  device->data = (void *)hyper;

  return 0;

error:
  __pi_hyperram_free(hyper);
  return -1;
}



void pi_hyperram_close(struct pi_device *device)
{
  pi_hyperram_t *hyper = (pi_hyperram_t *)device->data;
  __pi_hyperram_free(hyper);
}



uint32_t pi_hyperram_alloc(struct pi_device *device, uint32_t size)
{
  pi_hyperram_t *hyper = (pi_hyperram_t *)device->data;
  return (uint32_t)rt_extern_alloc(hyper->alloc, size);
}



int pi_hyperram_free(struct pi_device *device, uint32_t chunk, uint32_t size)
{
  pi_hyperram_t *hyper = (pi_hyperram_t *)device->data;
  return rt_extern_free(hyper->alloc, (void *)chunk, size);
}



void pi_hyperram_read_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, struct pi_fc_task *task)
{
  rt_hyperram_t *hyper = (rt_hyperram_t *)device->data;
  task->done = 0;
  __pi_hyper_copy(UDMA_CHANNEL_ID(hyper->channel) + 0, (uint32_t)addr, hyper_addr, size, task, REG_MBR0);
}



void pi_hyperram_read(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size)
{
  struct pi_fc_task task;
  pi_hyperram_read_async(device, hyper_addr, addr, size, mc_task(&task));
  mc_wait_on_task(&task);
}



void pi_hyperram_write_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, struct pi_fc_task *task)
{
  rt_hyperram_t *hyper = (rt_hyperram_t *)device->data;
  task->done = 0;
  __pi_hyper_copy(UDMA_CHANNEL_ID(hyper->channel) + 1, (uint32_t)addr, hyper_addr, size, task, REG_MBR0);
}



void pi_hyperram_write(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size)
{
  struct pi_fc_task task;
  pi_hyperram_write_async(device, hyper_addr, addr, size, mc_task(&task));
  mc_wait_on_task(&task);
}


void pi_hyperram_read_2d_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, struct pi_fc_task *task)
{
  rt_hyperram_t *hyper = (rt_hyperram_t *)device->data;
  task->done = 0;
  __pi_hyper_copy_2d(UDMA_CHANNEL_ID(hyper->channel) + 0, (uint32_t)addr, hyper_addr, size, stride, length, task, REG_MBR0);
}



void pi_hyperram_read_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length)
{
  struct pi_fc_task task;
  pi_hyperram_read_2d_async(device, hyper_addr, addr, size, stride, length, mc_task(&task));
  mc_wait_on_task(&task);
}



void pi_hyperram_write_2d_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, struct pi_fc_task *task)
{
  rt_hyperram_t *hyper = (rt_hyperram_t *)device->data;
  task->done = 0;
  __pi_hyper_copy_2d(UDMA_CHANNEL_ID(hyper->channel) + 1, (uint32_t)addr, hyper_addr, size, stride, length, task, REG_MBR0);
}



void pi_hyperram_write_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length)
{
  struct pi_fc_task task;
  pi_hyperram_write_2d_async(device, hyper_addr, addr, size, stride, length, mc_task(&task));
  mc_wait_on_task(&task);
}



static pi_hyperram_t *__pi_hyperram_init(int ramsize)
{
  pi_hyperram_t *hyper = rt_alloc(RT_ALLOC_FC_DATA, sizeof(pi_hyperram_t));
  if (hyper == NULL) goto error;

  hyper->alloc = NULL;

  __pi_hyper_temp_buffer = rt_alloc(RT_ALLOC_PERIPH, __PI_HYPER_TEMP_BUFFER_SIZE);
  if (__pi_hyper_temp_buffer == NULL) goto error;

  rt_extern_alloc_t *alloc = (rt_extern_alloc_t *)rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_extern_alloc_t));
  if (alloc == NULL) goto error;

  hyper->alloc = alloc;
  hyper->alloc_init = 0;
  if (rt_extern_alloc_init(alloc, 0, ramsize)) goto error;
  hyper->alloc_init = 1;

  return hyper;

error:
  __pi_hyperram_free(hyper);
  return NULL;
}



static void __pi_hyperram_free(pi_hyperram_t *hyper)
{
  if (__pi_hyper_temp_buffer != NULL)
  {
    rt_free(RT_ALLOC_PERIPH, __pi_hyper_temp_buffer, __PI_HYPER_TEMP_BUFFER_SIZE);
    __pi_hyper_temp_buffer = NULL;
  }

  if (hyper != NULL)
  {
    if (hyper->alloc != NULL)
    {
      if (hyper->alloc_init)
        rt_extern_alloc_deinit(hyper->alloc);

      rt_free(RT_ALLOC_FC_DATA, (void *)hyper, sizeof(rt_extern_alloc_t));
    }
    rt_free(RT_ALLOC_FC_DATA, (void *)hyper, sizeof(pi_hyperram_t));
  }
}



static void __attribute__((noinline)) __pi_hyper_copy_aligned(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event)
{
  unsigned int base = hal_udma_channel_base(channel);

  // In case the size is bigger than the maximum burst size
  // split the transfer into smaller transfers using the repeat count
  if (size > 512) {
    __rt_hyper_pending_base = base;
    __rt_hyper_pending_hyper_addr = hyper_addr;
    __rt_hyper_pending_addr = (unsigned int)addr;
    __rt_hyper_pending_repeat = 512;
    __rt_hyper_pending_repeat_size = size;
    size = 512;
  } else {
    __rt_hyper_pending_repeat = 0;
  }

  __rt_hyper_end_task = event;

  hal_hyper_enqueue(base, (unsigned int)addr, hyper_addr, size, UDMA_CHANNEL_CFG_EN);
}



static int __pi_hyper_resume_misaligned_read(struct pi_fc_task *task)
{
  while (1)
  {
    // Compute information to see how to do one more step
    int addr_aligned = (__rt_hyper_pending_emu_addr + 3) & ~0x3;
    int prologue_size = addr_aligned - (int)__rt_hyper_pending_emu_addr;
    int hyper_addr_aligned = __rt_hyper_pending_emu_hyper_addr + prologue_size;

    if (__rt_hyper_pending_emu_size < 4)
      prologue_size = __rt_hyper_pending_emu_size;

    if (prologue_size)
    {
      // Case where we have a partial copy to do
      if (!__rt_hyper_pending_emu_do_memcpy)
      {
        // A partial transfer must first transfer to the temporary area
        // and finish the transfer by hands using a memcpy.
        // This part is first called to trigger the transfer while the part after
        // is called to do the memcpy as a second step.
        __pi_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          (uint32_t)__pi_hyper_temp_buffer,
          (__rt_hyper_pending_emu_hyper_addr & ~1),
          4,
          NULL
        );

        // It is asynchronous, just remember we have to do
        // a memcpy when the transfer is done and leave
        __rt_hyper_pending_emu_do_memcpy = 1;
        return 0;
      }

      __rt_hyper_pending_emu_do_memcpy = 0;
      memcpy((void *)__rt_hyper_pending_emu_addr, &__pi_hyper_temp_buffer[__rt_hyper_pending_emu_hyper_addr & 1], prologue_size);

      __rt_hyper_pending_emu_hyper_addr += prologue_size;
      __rt_hyper_pending_emu_addr += prologue_size;
      __rt_hyper_pending_emu_size -= prologue_size;

      // The transfer is asynchronous, we get there to do the memcpy
      // without triggering any transfer, so we can start again to trigger one.
      if (__rt_hyper_pending_emu_size)
        continue;

    }
    else if (__rt_hyper_pending_emu_size > 0)
    {
      // Case where we have the body to transfer
      uint32_t size_aligned = __rt_hyper_pending_emu_size & ~0x3;

      if ((hyper_addr_aligned & 0x1) == 0)
      {
        // Good case where the body is aligned on both sides and we can do
        // a direct copy.
        __pi_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          __rt_hyper_pending_emu_addr,
          __rt_hyper_pending_emu_hyper_addr,
          size_aligned,
          NULL
        );

        __rt_hyper_pending_emu_hyper_addr += size_aligned;
        __rt_hyper_pending_emu_addr += size_aligned;
        __rt_hyper_pending_emu_size -= size_aligned;

        // It is asynchronous, just leave, we'll continue the transfer
        // when this one is over
        return 0;
      }
      else
      {
        // Bad case where we have to transfer the body using a temporary
        // buffer as the aligments on both sides are not compatible.
        // This part is very similar to the prologue.
        // Just be careful to split into small transfers to fit the temporary buffer.

        if (size_aligned > __PI_HYPER_TEMP_BUFFER_SIZE - 4)
          size_aligned = __PI_HYPER_TEMP_BUFFER_SIZE - 4;

        if (!__rt_hyper_pending_emu_do_memcpy)
        {
          __pi_hyper_copy_aligned(
            __rt_hyper_pending_emu_channel,
            (uint32_t)__pi_hyper_temp_buffer,
            (__rt_hyper_pending_emu_hyper_addr & ~1),
            size_aligned+4,
            NULL
          );

          __rt_hyper_pending_emu_do_memcpy = 1;
          return 0;
        }

        __rt_hyper_pending_emu_do_memcpy = 0;
        memcpy((void *)__rt_hyper_pending_emu_addr, &__pi_hyper_temp_buffer[1], size_aligned);

        __rt_hyper_pending_emu_hyper_addr += size_aligned;
        __rt_hyper_pending_emu_addr += size_aligned;
        __rt_hyper_pending_emu_size -= size_aligned;

        if (__rt_hyper_pending_emu_size)
          continue;
      }
    }

    // Now check if we are done
    if (__rt_hyper_pending_emu_size == 0)
    {
      // Check if we are doing a 2D transfer
      if (__rt_hyper_pending_emu_size_2d > 0)
      {
        // In this case, update the global size
        if (__rt_hyper_pending_emu_size_2d > __rt_hyper_pending_emu_length)
          __rt_hyper_pending_emu_size_2d -= __rt_hyper_pending_emu_length;
        else
          __rt_hyper_pending_emu_size_2d = 0;

        // And check if we must reenqueue a line.
        if (__rt_hyper_pending_emu_size_2d > 0)
        {
          __rt_hyper_pending_emu_hyper_addr = __rt_hyper_pending_emu_hyper_addr - __rt_hyper_pending_emu_length + __rt_hyper_pending_emu_stride;
          __rt_hyper_pending_emu_size = __rt_hyper_pending_emu_size_2d > __rt_hyper_pending_emu_length ? __rt_hyper_pending_emu_length : __rt_hyper_pending_emu_size_2d;
          continue;
        }
      }

      __rt_hyper_pending_emu_task = NULL;
      rt_event_enqueue(task);

      return 1;
    }
    break;
  }

  return 0;
}



static int __pi_hyper_resume_misaligned_write(struct pi_fc_task *task)
{

  while(1)
  {
    // Compute information to see how to do one more step
    int addr_aligned = (__rt_hyper_pending_emu_addr + 3) & ~0x3;
    int prologue_size = addr_aligned - __rt_hyper_pending_emu_addr;
    int hyper_addr_aligned = __rt_hyper_pending_emu_hyper_addr + prologue_size;

    if (__rt_hyper_pending_emu_size < 4)
      prologue_size = __rt_hyper_pending_emu_size;

    if (prologue_size)
    {
      // Case where we have a partial copy to do
      if (!__rt_hyper_pending_emu_do_memcpy)
      {
        // A partial transfer must first transfer the content of the hyperram
        // to the temporary area and partially overwrite it with a memcpy.
        // This part is first called to trigger the transfer while the part after
        // is called to do the memcpy and the final transfer as a second step.
        __pi_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel-1,
          (uint32_t)__pi_hyper_temp_buffer,
          (__rt_hyper_pending_emu_hyper_addr & ~1),
          4,
          NULL
        );

        // It is asynchronous, just remember we have to do
        // a memcpy when the transfer is done and leave
        __rt_hyper_pending_emu_do_memcpy = 1;
        return 0;
      }

      __rt_hyper_pending_emu_do_memcpy = 0;
      memcpy(&__pi_hyper_temp_buffer[__rt_hyper_pending_emu_hyper_addr & 1], (void *)__rt_hyper_pending_emu_addr, prologue_size);

      __pi_hyper_copy_aligned(
        __rt_hyper_pending_emu_channel,
        (uint32_t)__pi_hyper_temp_buffer,
        (__rt_hyper_pending_emu_hyper_addr & ~1),
        4,
        NULL
      );

      __rt_hyper_pending_emu_hyper_addr += prologue_size;
      __rt_hyper_pending_emu_addr += prologue_size;
      __rt_hyper_pending_emu_size -= prologue_size;

      return 0;
    }
    else if (__rt_hyper_pending_emu_size > 0)
    {
      // Case where we have the body to transfer
      uint32_t size_aligned = __rt_hyper_pending_emu_size & ~0x3;

      if ((hyper_addr_aligned & 0x1) == 0)
      {
        // Good case where the body is aligned on both sides and we can do
        // a direct copy.
        __pi_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          __rt_hyper_pending_emu_addr,
          __rt_hyper_pending_emu_hyper_addr,
          size_aligned,
          NULL
        );

        __rt_hyper_pending_emu_hyper_addr += size_aligned;
        __rt_hyper_pending_emu_addr += size_aligned;
        __rt_hyper_pending_emu_size -= size_aligned;

        // It is asynchronous, just leave, we'll continue the transfer
        // when this one is over
        return 0;
      }
      else
      {
        // Bad case where we have to transfer the body using a temporary
        // buffer as the aligments on both sides are not compatible.
        // This part is very similar to the prologue.
        // Just be careful to split into small transfers to fit the temporary buffer.

        if (size_aligned > __PI_HYPER_TEMP_BUFFER_SIZE - 4)
          size_aligned = __PI_HYPER_TEMP_BUFFER_SIZE - 4;

        if (!__rt_hyper_pending_emu_do_memcpy)
        {
          __pi_hyper_copy_aligned(
            __rt_hyper_pending_emu_channel-1,
            (uint32_t)__pi_hyper_temp_buffer,
            (__rt_hyper_pending_emu_hyper_addr & ~1),
            4,
            NULL
          );

          __rt_hyper_pending_emu_do_memcpy = 1;
          return 0;
        }

        __rt_hyper_pending_emu_do_memcpy = 0;
        memcpy(&__pi_hyper_temp_buffer[1], (void *)__rt_hyper_pending_emu_addr, size_aligned-1);

        __pi_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          (uint32_t)__pi_hyper_temp_buffer,
          (__rt_hyper_pending_emu_hyper_addr & ~1),
          size_aligned,
          NULL
        );

        __rt_hyper_pending_emu_hyper_addr += size_aligned-1;
        __rt_hyper_pending_emu_addr += size_aligned-1;
        __rt_hyper_pending_emu_size -= size_aligned-1;

        return 0;
      }
    }

    // Now check if we are done
    if (__rt_hyper_pending_emu_size == 0)
    {
      // Check if we are doing a 2D transfer
      if (__rt_hyper_pending_emu_size_2d > 0)
      {
        // In this case, update the global size
        if (__rt_hyper_pending_emu_size_2d > __rt_hyper_pending_emu_length)
          __rt_hyper_pending_emu_size_2d -= __rt_hyper_pending_emu_length;
        else
          __rt_hyper_pending_emu_size_2d = 0;

        // And check if we must reenqueue a line.
        if (__rt_hyper_pending_emu_size_2d > 0)
        {
          __rt_hyper_pending_emu_hyper_addr = __rt_hyper_pending_emu_hyper_addr - __rt_hyper_pending_emu_length + __rt_hyper_pending_emu_stride;
          __rt_hyper_pending_emu_size = __rt_hyper_pending_emu_size_2d > __rt_hyper_pending_emu_length ? __rt_hyper_pending_emu_length : __rt_hyper_pending_emu_size_2d;
          continue;
        }
      }

      __rt_hyper_pending_emu_task = NULL;
      rt_event_enqueue(task);

      return 1;
    }
    break;
  }

  return 0;
}



static void __pi_hyper_copy_misaligned(struct pi_fc_task *task)
{
    int end;
    if (__rt_hyper_pending_emu_channel & 1)
      end = __pi_hyper_resume_misaligned_write(task);
    else
      end = __pi_hyper_resume_misaligned_read(task);

    if (!end)
      return;

    exec_pending_task();
}



static void __pi_hyper_copy_exec(int channel, uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event)
{
  // Check if we are in the fast case where everything is correctly aligned.
  if (likely((((int)addr & 0x3) == 0) && (((int)hyper_addr) & 0x1) == 0 && (((int)size & 0x3) == 0)))
  {
    __pi_hyper_copy_aligned(channel, addr, hyper_addr, size, event);
  }
  else
  {
    // Otherwise go through the slow misaligned case.
    __rt_hyper_pending_emu_channel = channel;
    __rt_hyper_pending_emu_hyper_addr = (unsigned int)hyper_addr;
    __rt_hyper_pending_emu_addr = (unsigned int)addr;
    __rt_hyper_pending_emu_size = size;
    __rt_hyper_pending_emu_do_memcpy = 0;
    __rt_hyper_pending_emu_task = event;

    __pi_hyper_copy_misaligned(event);
  }
}



static void __pi_hyper_2d_copy_exec(int channel, uint32_t addr, uint32_t hyper_addr, uint32_t size, int stride, uint32_t length, rt_event_t *event)
{
  // Otherwise go through the slow misaligned case.
  __rt_hyper_pending_emu_channel = channel;
  __rt_hyper_pending_emu_hyper_addr = hyper_addr;
  __rt_hyper_pending_emu_addr = (unsigned int)addr;
  __rt_hyper_pending_emu_size = size > length ? length : size;
  __rt_hyper_pending_emu_do_memcpy = 0;
  __rt_hyper_pending_emu_task = event;
  __rt_hyper_pending_emu_size_2d = size;
  __rt_hyper_pending_emu_length = length;
  __rt_hyper_pending_emu_stride = stride;

  __pi_hyper_copy_misaligned(event);
}



static void exec_pending_task()
{
  struct pi_fc_task *task = __rt_hyper_pending_tasks;

  if (task)
  {
    __rt_hyper_pending_tasks = task->implem.next;

    int is_2d = (task->implem.data[0] >> 8) & 0xff;
    unsigned int channel = task->implem.data[0] & 0xff;
    uint32_t addr = task->implem.data[1];
    uint32_t hyper_addr = task->implem.data[2];
    uint32_t size = task->implem.data[3];

    if (!is_2d)
    {
      __pi_hyper_copy_exec(channel, addr, hyper_addr, size, task);
    }
    else
    {
      uint32_t stride = task->implem.data[4];
      uint32_t length = task->implem.data[5];
      __pi_hyper_2d_copy_exec(channel, addr, hyper_addr, size, stride, length, task);
    }
  }
}



void __pi_hyper_copy(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event, int mbr)
{
  int irq = rt_irq_disable();

  hyper_addr |= mbr;

  if (__rt_hyper_end_task != NULL || __rt_hyper_pending_emu_size != 0)
  {
    if (__rt_hyper_pending_tasks != NULL)
    __rt_hyper_pending_tasks_last->implem.next = event;
    else
      __rt_hyper_pending_tasks = event;
    __rt_hyper_pending_tasks_last = event;
    event->implem.next = NULL;

    event->implem.data[0] = channel;
    event->implem.data[1] = (unsigned int)addr;
    event->implem.data[2] = (unsigned int)hyper_addr;
    event->implem.data[3] = size;
  }
  else
  {
    __pi_hyper_copy_exec(channel, addr, hyper_addr, size, event);
  }

  rt_irq_restore(irq);
}



void __pi_hyper_copy_2d(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, int stride, int length, rt_event_t *event, int mbr)
{
  int irq = rt_irq_disable();

  hyper_addr |= mbr;

  if (__rt_hyper_end_task != NULL || __rt_hyper_pending_emu_size_2d != 0)
  {
    if (__rt_hyper_pending_tasks != NULL)
    __rt_hyper_pending_tasks_last->implem.next = event;
    else
      __rt_hyper_pending_tasks = event;
    __rt_hyper_pending_tasks_last = event;
    event->implem.next = NULL;

    event->implem.data[0] = channel | (1<<8);
    event->implem.data[1] = (unsigned int)addr;
    event->implem.data[2] = (unsigned int)hyper_addr;
    event->implem.data[3] = size;
    event->implem.data[4] = stride;
    event->implem.data[5] = length;
  }
  else
  {
    __pi_hyper_2d_copy_exec(channel, addr, hyper_addr, size, stride, length, event);
  }

  rt_irq_restore(irq);
}



void __rt_hyper_resume_emu_task()
{
  // The pending copy is an emulated misaligned request, just continue it
  __pi_hyper_copy_misaligned(__rt_hyper_pending_emu_task);
}



void __rt_hyper_resume_copy(struct pi_fc_task *task)
{
  exec_pending_task();
}



static RT_BOOT_CODE void __attribute__((constructor)) __rt_hyper_init()
{
  __pi_hyper_temp_buffer = NULL;
  __rt_hyper_end_task = NULL;
  __rt_hyper_pending_tasks = NULL;
  __pi_hyper_cluster_reqs_first = NULL;
  __rt_hyper_pending_emu_channel = -1;
}



#if defined(ARCHI_HAS_CLUSTER)

static void __pi_hyperram_cluster_req_exec(pi_cl_hyperram_req_t *req)
{
  pi_hyperram_t *hyper = (pi_hyperram_t *)req->device->data;
  rt_event_t *event = &__pi_hyper_cluster_task;
  mc_task_callback(event, __pi_hyperram_cluster_req_done, (void* )req);

  if(req->is_2d)
    __pi_hyper_copy_2d(UDMA_CHANNEL_ID(hyper->channel) + req->is_write, (uint32_t)req->addr, req->hyper_addr, req->size, req->stride, req->length, event, REG_MBR0);
  else
    __pi_hyper_copy(UDMA_CHANNEL_ID(hyper->channel) + req->is_write, (uint32_t)req->addr, req->hyper_addr, req->size, event, REG_MBR0);
}

static void __pi_hyperram_cluster_req_done(void *_req)
{
  pi_cl_hyperram_req_t *req = (pi_cl_hyperram_req_t *)_req;
  req->done = 1;
  __rt_cluster_notif_req_done(req->cid);
    __pi_hyper_cluster_reqs_first = req->next;

  req = __pi_hyper_cluster_reqs_first;
  if (req)
  {
    __pi_hyper_cluster_reqs_first = req->next;
    __pi_hyperram_cluster_req_exec(req);
  }
}

static void __pi_hyperram_cluster_req(void *_req)
{
  pi_cl_hyperram_req_t *req = (pi_cl_hyperram_req_t* )_req;

  int is_first = __pi_hyper_cluster_reqs_first == NULL;

  if (is_first)
    __pi_hyper_cluster_reqs_first = req;
  else
    __pi_hyper_cluster_reqs_last->next = req;

  __pi_hyper_cluster_reqs_last = req;
  req->next = NULL;

  if (is_first)
  {
    __pi_hyperram_cluster_req_exec(req);
  }
}

void __cl_hyperram_cluster_copy(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, pi_cl_hyperram_req_t *req, int ext2loc)
{
  req->device = device;
  req->addr = addr;
  req->hyper_addr = hyper_addr;
  req->size = size;
  req->cid = rt_cluster_id();
  req->done = 0;
  req->is_write = (ext2loc)? 0:1;
  req->is_2d = 0;
  mc_task_callback(&req->event, __pi_hyperram_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}


void __cl_hyperram_cluster_copy_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, pi_cl_hyperram_req_t *req, int ext2loc)
{
  req->device = device;
  req->addr = addr;
  req->hyper_addr = hyper_addr;
  req->size = size;
  req->stride = stride;
  req->length = length;
  req->cid = rt_cluster_id();
  req->done = 0;
  req->is_write = (ext2loc)? 0:1;
  req->is_2d = 1;
  mc_task_callback(&req->event, __pi_hyperram_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}


void __pi_hyperram_alloc_cluster_req(void *_req)
{
  pi_cl_hyperram_alloc_req_t *req = (pi_cl_hyperram_alloc_req_t *)_req;
  req->result = pi_hyperram_alloc(req->device, req->size);
  req->done = 1;
  __rt_cluster_notif_req_done(req->cid);
}



void __pi_hyperram_free_cluster_req(void *_req)
{
  pi_cl_hyperram_free_req_t *req = (pi_cl_hyperram_free_req_t *)_req;
  pi_hyperram_free(req->device, req->chunk, req->size);
  req->done = 1;
  __rt_cluster_notif_req_done(req->cid);
}



void pi_cl_hyperram_alloc(struct pi_device *device, uint32_t size, pi_cl_hyperram_alloc_req_t *req)
{
  req->device = device;
  req->size = size;
  req->cid = rt_cluster_id();
  req->done = 0;
  mc_task_callback(&req->event, __pi_hyperram_alloc_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}



void pi_cl_hyperram_free(struct pi_device *device, uint32_t chunk, uint32_t size, pi_cl_hyperram_free_req_t *req)
{
  req->device = device;
  req->size = size;
  req->chunk = chunk;
  req->cid = rt_cluster_id();
  req->done = 0;
  mc_task_callback(&req->event, __pi_hyperram_free_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}

#endif
