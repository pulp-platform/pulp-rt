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


RT_FC_TINY_DATA struct fc_task *__rt_hyper_end_task;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_base;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_hyper_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_repeat;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_repeat_size;

RT_FC_TINY_DATA struct fc_task *__rt_hyper_pending_tasks;
RT_FC_TINY_DATA struct fc_task *__rt_hyper_pending_tasks_last;

RT_FC_TINY_DATA int __rt_hyper_pending_emu_channel;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_hyper_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_addr;
RT_FC_TINY_DATA unsigned int __rt_hyper_pending_emu_size;
RT_FC_TINY_DATA unsigned char __rt_hyper_pending_emu_do_memcpy;

static struct fc_task __mc_hyper_cluster_task;


typedef struct {
  rt_extern_alloc_t *alloc;
  int channel;
} mc_hyperram_t;


extern void __rt_hyper_handle_copy();



#define __MC_HYPER_TEMP_BUFFER_SIZE 128

// Temporary buffer of size __MC_HYPER_TEMP_BUFFER_SIZE used for misaligned
// transfers between hyperram and L2
static char *__mc_hyper_temp_buffer;



void __mc_hyper_copy_2d(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, int stride, int length, rt_event_t *event, int mbr);


void __mc_hyper_copy(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event, int mbr);


int __mc_hyperram_init(mc_hyperram_t *hyper, int ramsize)
{
  rt_extern_alloc_t *alloc = (rt_extern_alloc_t *)rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_extern_alloc_t));
  if (alloc == NULL) return -1;

  hyper->alloc = alloc;
  if (rt_extern_alloc_init(alloc, 0, ramsize)) return -1;

  return 0;
}



void mc_hyperram_conf_init(struct mc_hyperram_conf *conf)
{
  conf->id = -1;
  conf->ram_size = 0;
}



static void __mc_hyperram_free(mc_hyperram_t *hyper)
{
  if (hyper != NULL) {
    if (hyper->alloc != NULL) rt_free(RT_ALLOC_FC_DATA, (void *)hyper, sizeof(rt_extern_alloc_t));
    rt_free(RT_ALLOC_FC_DATA, (void *)hyper, sizeof(mc_hyperram_t));
  }
}



int mc_hyperram_open(struct pmsis_device *device, struct mc_hyperram_conf *conf)
{
  mc_hyperram_t *hyper = NULL;
  int periph_id;
  int channel;
  int ramsize;

  periph_id = ARCHI_UDMA_HYPER_ID(conf->id);
  channel = UDMA_EVENT_ID(periph_id);
  ramsize = conf->ram_size;

  if (__mc_hyper_temp_buffer == NULL)
  {
    __mc_hyper_temp_buffer = rt_alloc(RT_ALLOC_PERIPH, __MC_HYPER_TEMP_BUFFER_SIZE);
    if (__mc_hyper_temp_buffer == NULL) goto error;
  }

  hyper = rt_alloc(RT_ALLOC_FC_DATA, sizeof(mc_hyperram_t));
  if (hyper == NULL) goto error;

  hyper->alloc = NULL;
  hyper->channel = periph_id;

  if (__mc_hyperram_init(hyper, ramsize)) goto error;

  soc_eu_fcEventMask_setEvent(channel);
  soc_eu_fcEventMask_setEvent(channel+1);
  plp_udma_cg_set(plp_udma_cg_get() | (1<<periph_id));

  __rt_periph_channel(channel+1)->callback = __rt_hyper_handle_copy;
  __rt_periph_channel(channel)->callback = __rt_hyper_handle_copy;

  device->data = (void *)hyper;

  return 0;

error:
  __mc_hyperram_free(hyper);
  return -1;
}



void mc_hyperram_close(struct pmsis_device *device)
{
  mc_hyperram_t *hyper = (mc_hyperram_t *)device->data;
  __mc_hyperram_free(hyper);
}



#if defined(ARCHI_HAS_CLUSTER)

void __mc_hyperram_cluster_req_done(void *_req)
{
  cl_hyperram_req_t *req = (cl_hyperram_req_t *)_req;
  req->done = 1;
  __rt_cluster_notif_req_done(req->cid);
}

void __mc_hyperram_cluster_req(void *_req)
{
 cl_hyperram_req_t *req = (cl_hyperram_req_t* )_req;
 mc_hyperram_t *hyper = (mc_hyperram_t *)req->device->data;
 // TODO to support several requests, we should check if the global task is available
 rt_event_t *event = &__mc_hyper_cluster_task;
 mc_task_callback(event, __mc_hyperram_cluster_req_done, (void* )req);
 if(req->is_2d)
     __mc_hyper_copy_2d(UDMA_CHANNEL_ID(hyper->channel) + req->is_write, (uint32_t)req->addr, req->hyper_addr, req->size, req->stride, req->length, event, REG_MBR0);
 else
     __mc_hyper_copy(UDMA_CHANNEL_ID(hyper->channel) + req->is_write, (uint32_t)req->addr, req->hyper_addr, req->size, event, REG_MBR0);
}

void __mc_hyperram_cluster_copy(struct pmsis_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, cl_hyperram_req_t *req, int ext2loc)
{
  req->device = device;
  req->addr = addr;
  req->hyper_addr = hyper_addr;
  req->size = size;
  req->cid = rt_cluster_id();
  req->done = 0;
  req->is_write = (ext2loc)? 0:1;
  req->is_2d = 0;
  mc_task_callback(&req->event, __mc_hyperram_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}


void __mc_hyperram_cluster_copy_2d(struct pmsis_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, int stride, int length, cl_hyperram_req_t *req, int ext2loc)
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
  mc_task_callback(&req->event, __mc_hyperram_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}


void __mc_hyperram_alloc_cluster_req(void *_req)
{
  cl_hyperram_alloc_req_t *req = (cl_hyperram_alloc_req_t *)_req;
  req->result = mc_hyperram_alloc(req->device, req->size);
  req->done = 1;
  __rt_cluster_notif_req_done(req->cid);
}



void __mc_hyperram_free_cluster_req(void *_req)
{
  cl_hyperram_free_req_t *req = (cl_hyperram_free_req_t *)_req;
  mc_hyperram_free(req->device, req->chunk, req->size);
  req->done = 1;
  __rt_cluster_notif_req_done(req->cid);
}



void cl_hyperram_alloc_cluster(struct pmsis_device *device, uint32_t size, cl_hyperram_alloc_req_t *req)
{
  req->device = device;
  req->size = size;
  req->cid = rt_cluster_id();
  req->done = 0;
  mc_task_callback(&req->event, __mc_hyperram_alloc_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}



void cl_hyperram_free_cluster(struct pmsis_device *device, uint32_t chunk, uint32_t size, cl_hyperram_free_req_t *req)
{
  req->device = device;
  req->size = size;
  req->chunk = chunk;
  req->cid = rt_cluster_id();
  req->done = 0;
  mc_task_callback(&req->event, __mc_hyperram_free_cluster_req, (void* )req);
  __rt_cluster_push_fc_event(&req->event);
}

#endif


// Performs a direct aligned copy:
//  - hyper addr is multiple of 2
//  - l2 addr is multiple of 4
//  - size is multiple of 4
void __attribute__((noinline)) __mc_hyper_copy_aligned(int channel,
  uint32_t addr, uint32_t _hyper_addr, uint32_t size, rt_event_t *event, int mbr)
{
  unsigned int hyper_addr = mbr | _hyper_addr;
  unsigned int base = hal_udma_channel_base(channel);


  //printf("Copy aligned (channel: %d, addr: 0x%lx, hyper_addr: 0x%x, size: 0x%lx, event: %p)\n", channel, addr, hyper_addr, size, event);

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
  if (event)
    event->implem.next = NULL;
  hal_hyper_enqueue(base, (unsigned int)addr, hyper_addr, size, UDMA_CHANNEL_CFG_EN);
}




// Performs a misaligned 2d read without any constraint.
// This function can be either called directly or as an event callback
// This function is like a state machine,
// it checks the state of the pending copy and does one more step
// so that the whole transfer can be done asynchronously without blocking
// the core.
static void __mc_hyper_resume_misaligned_read(struct fc_task *task)
{
  while (1)
  {
    // Compute information to see how to do one more step
    int addr_aligned = (__rt_hyper_pending_emu_addr + 3) & ~0x3;
    int prologue_size = addr_aligned - (int)__rt_hyper_pending_emu_addr;
    int hyper_addr_aligned = __rt_hyper_pending_emu_hyper_addr + prologue_size;

    //printf("Resuming misaligned read (channel: %d, addr: 0x%x, hyper_addr: 0x%x, size: 0x%x@%p)\n", __rt_hyper_pending_emu_channel, __rt_hyper_pending_emu_addr, __rt_hyper_pending_emu_hyper_addr, __rt_hyper_pending_emu_size, &__rt_hyper_pending_emu_size);

    if (__rt_hyper_pending_emu_size < 4)
      prologue_size = __rt_hyper_pending_emu_size;

    if (prologue_size)
    {
      //printf("Handling prologue\n");
      // Case where we have a partial copy to do
      if (!__rt_hyper_pending_emu_do_memcpy)
      {
        // A partial transfer must first transfer to the temporary area
        // and finish the transfer by hands using a memcpy.
        // This part is first called to trigger the transfer while the part after
        // is called to do the memcpy as a second step.
        __mc_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          (uint32_t)__mc_hyper_temp_buffer,
          (__rt_hyper_pending_emu_hyper_addr & ~1),
          4,
          NULL, 0
        );

        // It is asynchronous, just remember we have to do
        // a memcpy when the transfer is done and leave
        __rt_hyper_pending_emu_do_memcpy = 1;
        return;
      }

      __rt_hyper_pending_emu_do_memcpy = 0;
      memcpy((void *)__rt_hyper_pending_emu_addr, &__mc_hyper_temp_buffer[__rt_hyper_pending_emu_hyper_addr & 1], prologue_size);

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
      //printf("Handling body\n");
        // Good case where the body is aligned on both sides and we can do
        // a direct copy.
        __mc_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          __rt_hyper_pending_emu_addr,
          __rt_hyper_pending_emu_hyper_addr,
          size_aligned,
          NULL, 0
        );

        __rt_hyper_pending_emu_hyper_addr += size_aligned;
        __rt_hyper_pending_emu_addr += size_aligned;
        __rt_hyper_pending_emu_size -= size_aligned;

        // It is asynchronous, just leave, we'll continue the transfer
        // when this one is over
        return;
      }
      else
      {
      //printf("Handling epilogue\n");
        // Bad case where we have to transfer the body using a temporary
        // buffer as the aligments on both sides are not compatible.
        // This part is very similar to the prologue.
        // Just be careful to split into small transfers to fit the temporary buffer.

        if (size_aligned > __MC_HYPER_TEMP_BUFFER_SIZE - 4)
          size_aligned = __MC_HYPER_TEMP_BUFFER_SIZE - 4;

        if (!__rt_hyper_pending_emu_do_memcpy)
        {
      //printf("Trigger transfer\n");
          __mc_hyper_copy_aligned(
            __rt_hyper_pending_emu_channel,
            (uint32_t)__mc_hyper_temp_buffer,
            (__rt_hyper_pending_emu_hyper_addr & ~1),
            size_aligned+4,
            NULL, 0
          );

          __rt_hyper_pending_emu_do_memcpy = 1;
          return;
        }

      //printf("Do memcpy\n");
        __rt_hyper_pending_emu_do_memcpy = 0;
        memcpy((void *)__rt_hyper_pending_emu_addr, &__mc_hyper_temp_buffer[1], size_aligned);

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
      struct fc_task *task = __rt_hyper_pending_tasks;
      __rt_hyper_pending_tasks = task->implem.next;

      rt_event_enqueue(task);



      #if 0
      // Check if we are doing a 2D transfer
      if (copy->u.hyper.size_2d > 0)
      {
        // In this case, update the global size
        copy->u.hyper.size_2d -= copy->u.hyper.length;

        // And check if we must reenqueue a line.
        if (copy->u.hyper.size_2d > 0)
        {
          copy->u.hyper.pending_hyper_addr = copy->u.hyper.pending_hyper_addr - copy->u.hyper.length + copy->u.hyper.stride;
          copy->u.hyper.pending_size = copy->u.hyper.size_2d > copy->u.hyper.length ? copy->u.hyper.length : copy->u.hyper.size_2d;
          if (!async) event = __rt_wait_event_prepare_blocking();
          goto start;
        }
      }
      #endif
    }
    break;
  }
}



// Performs a misaligned 2d write without any constraint.
// This function can be either called directly or as an event callback
// This function is like a state machine,
// it checks the state of the pending copy and does one more step
// so that the whole transfer can be done asynchronously without blocking
// the core.
static void __mc_hyper_resume_misaligned_write(struct fc_task *task)
{

  while(1)
  {
    // Compute information to see how to do one more step
    int addr_aligned = (__rt_hyper_pending_emu_addr + 3) & ~0x3;
    int prologue_size = addr_aligned - __rt_hyper_pending_emu_addr;
    int hyper_addr_aligned = __rt_hyper_pending_emu_hyper_addr + prologue_size;

    //printf("Resuming misaligned write (channel: %d, addr: 0x%x, hyper_addr: 0x%x)\n", __rt_hyper_pending_emu_channel, __rt_hyper_pending_emu_addr, __rt_hyper_pending_emu_hyper_addr);

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
        __mc_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel-1,
          (uint32_t)__mc_hyper_temp_buffer,
          (__rt_hyper_pending_emu_hyper_addr & ~1),
          4,
          NULL, 0
        );

        // It is asynchronous, just remember we have to do
        // a memcpy when the transfer is done and leave
        __rt_hyper_pending_emu_do_memcpy = 1;
        return;
      }

      __rt_hyper_pending_emu_do_memcpy = 0;
      memcpy(&__mc_hyper_temp_buffer[__rt_hyper_pending_emu_hyper_addr & 1], (void *)__rt_hyper_pending_emu_addr, prologue_size);

      __mc_hyper_copy_aligned(
        __rt_hyper_pending_emu_channel,
        (uint32_t)__mc_hyper_temp_buffer,
        (__rt_hyper_pending_emu_hyper_addr & ~1),
        4,
        NULL, 0
      );

      __rt_hyper_pending_emu_hyper_addr += prologue_size;
      __rt_hyper_pending_emu_addr += prologue_size;
      __rt_hyper_pending_emu_size -= prologue_size;

      return;
    }
    else if (__rt_hyper_pending_emu_size > 0)
    {
      // Case where we have the body to transfer
      uint32_t size_aligned = __rt_hyper_pending_emu_size & ~0x3;

      if ((hyper_addr_aligned & 0x1) == 0)
      {
        // Good case where the body is aligned on both sides and we can do
        // a direct copy.
        __mc_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          __rt_hyper_pending_emu_addr,
          __rt_hyper_pending_emu_hyper_addr,
          size_aligned,
          NULL, 0
        );

        __rt_hyper_pending_emu_hyper_addr += size_aligned;
        __rt_hyper_pending_emu_addr += size_aligned;
        __rt_hyper_pending_emu_size -= size_aligned;

        // It is asynchronous, just leave, we'll continue the transfer
        // when this one is over
        return;
      }
      else
      {
        // Bad case where we have to transfer the body using a temporary
        // buffer as the aligments on both sides are not compatible.
        // This part is very similar to the prologue.
        // Just be careful to split into small transfers to fit the temporary buffer.

        if (size_aligned > __MC_HYPER_TEMP_BUFFER_SIZE - 4)
          size_aligned = __MC_HYPER_TEMP_BUFFER_SIZE - 4;

        if (!__rt_hyper_pending_emu_do_memcpy)
        {
          __mc_hyper_copy_aligned(
            __rt_hyper_pending_emu_channel-1,
            (uint32_t)__mc_hyper_temp_buffer,
            (__rt_hyper_pending_emu_hyper_addr & ~1),
            4,
            NULL, 0
          );

          __rt_hyper_pending_emu_do_memcpy = 1;
          return;
        }

        __rt_hyper_pending_emu_do_memcpy = 0;
        memcpy(&__mc_hyper_temp_buffer[1], (void *)__rt_hyper_pending_emu_addr, size_aligned-1);

        __mc_hyper_copy_aligned(
          __rt_hyper_pending_emu_channel,
          (uint32_t)__mc_hyper_temp_buffer,
          (__rt_hyper_pending_emu_hyper_addr & ~1),
          size_aligned,
          NULL, 0
        );

        __rt_hyper_pending_emu_hyper_addr += size_aligned-1;
        __rt_hyper_pending_emu_addr += size_aligned-1;
        __rt_hyper_pending_emu_size -= size_aligned-1;

        return;
      }
    }

    // Now check if we are done
    if (__rt_hyper_pending_emu_size == 0)
    {

      #if 0
      // Check if we are doing a 2D transfer
      if (copy->u.hyper.size_2d > 0)
      {
        // In this case, update the global size
        copy->u.hyper.size_2d -= copy->u.hyper.length;

        // And check if we must reenqueue a line.
        if (copy->u.hyper.size_2d > 0)
        {
          copy->u.hyper.pending_hyper_addr = copy->u.hyper.pending_hyper_addr - copy->u.hyper.length + copy->u.hyper.stride;
          copy->u.hyper.pending_size = copy->u.hyper.size_2d > copy->u.hyper.length ? copy->u.hyper.length : copy->u.hyper.size_2d;
          if (!async) event = __rt_wait_event_prepare_blocking();
          continue;
        }
      }
      #endif
    }
    break;
  }
}



static void __attribute__((noinline)) __mc_hyper_copy_misaligned(struct fc_task *task)
{
  if (__rt_hyper_pending_emu_channel & 1)
    __mc_hyper_resume_misaligned_write(task);
  else
    __mc_hyper_resume_misaligned_read(task);
}



void __mc_hyper_copy_exec(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event, int mbr)
{
  // Check if we are in the fast case where everything is correctly aligned.
  if (likely((((int)addr & 0x3) == 0) && (((int)hyper_addr) & 0x1) == 0 && (((int)size & 0x3) == 0)))
  {
    __mc_hyper_copy_aligned(channel, addr, hyper_addr, size, event, mbr);
  }
  else
  {
    // Otherwise go through the slow misaligned case.
    __rt_hyper_pending_emu_channel = channel;
    __rt_hyper_pending_emu_hyper_addr = (unsigned int)hyper_addr | mbr;
    __rt_hyper_pending_emu_addr = (unsigned int)addr;
    __rt_hyper_pending_emu_size = size;
    __rt_hyper_pending_emu_do_memcpy = 0;

    if (__rt_hyper_pending_tasks != NULL)
    __rt_hyper_pending_tasks_last->implem.next = event;
    else
      __rt_hyper_pending_tasks = event;
    __rt_hyper_pending_tasks_last = event;
    event->implem.next = NULL;

    __mc_hyper_copy_misaligned(event);
  }
}



void __mc_hyper_copy(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, rt_event_t *event, int mbr)
{
  int irq = rt_irq_disable();

    //printf("Hyper copy (channel: %d, addr: %lx, hyper_addr: 0x%lx)\n", channel, addr, hyper_addr);

  if (__rt_hyper_end_task != NULL || __rt_hyper_pending_tasks != NULL)
  {
    if (__rt_hyper_pending_tasks != NULL)
    __rt_hyper_pending_tasks_last->implem.next = event;
    else
      __rt_hyper_pending_tasks = event;
    __rt_hyper_pending_tasks_last = event;
    event->implem.next = NULL;

    event->implem.data[0] = channel | (mbr << 16);
    event->implem.data[1] = (unsigned int)addr;
    event->implem.data[2] = (unsigned int)hyper_addr;
    event->implem.data[3] = size;
  }
  else
  {
    __mc_hyper_copy_exec(channel, addr, hyper_addr, size, event, mbr);
  }

  rt_irq_restore(irq);
}

void __rt_hyper_resume_copy(struct fc_task *task)
{
  if (__rt_hyper_pending_emu_channel != -1)
  {
    // The pending copy is an emulated misaligned request, just continue it
    __mc_hyper_copy_misaligned(task);
  }
  else
  {
    unsigned int channel = task->implem.data[0] & 0xff;
    unsigned int mbr = (task->implem.data[0] >> 16) << 16;
    uint32_t addr = task->implem.data[1];
    uint32_t hyper_addr = task->implem.data[2];
    uint32_t size = task->implem.data[3];

    __mc_hyper_copy_exec(channel, addr, hyper_addr, size, task, mbr);

    // We must keep the pending request in the list only if it is an emulated misaligned request,
    // otherwise removed it
    if (__rt_hyper_pending_emu_channel == -1)
      __rt_hyper_pending_tasks = __rt_hyper_pending_tasks->implem.next;

  }
}


void __mc_hyper_copy_2d(int channel,
  uint32_t addr, uint32_t hyper_addr, uint32_t size, int stride, int length, rt_event_t *event, int mbr)
{
  int irq = rt_irq_disable();

  if (__rt_hyper_pending_tasks != NULL)
  __rt_hyper_pending_tasks_last->implem.next = event;
  else
    __rt_hyper_pending_tasks = event;
  __rt_hyper_pending_tasks_last = event;
  event->implem.next = NULL;

  event->implem.data[0] = channel | (mbr << 16);
  event->implem.data[1] = (unsigned int)addr;
  event->implem.data[2] = (unsigned int)hyper_addr;
  event->implem.data[3] = size;
  event->implem.data[4] = stride;
  event->implem.data[5] = length;

  __mc_hyper_copy_misaligned(event);

  rt_irq_restore(irq);
}



RT_FC_BOOT_CODE void __attribute__((constructor)) __mc_hyper_init()
{
  __mc_hyper_temp_buffer = NULL;
}



uint32_t mc_hyperram_alloc(struct pmsis_device *device, uint32_t size)
{
  mc_hyperram_t *hyper = (mc_hyperram_t *)device->data;
  return (uint32_t)rt_extern_alloc(hyper->alloc, size);
}

int mc_hyperram_free(struct pmsis_device *device, uint32_t chunk, uint32_t size)
{
  mc_hyperram_t *hyper = (mc_hyperram_t *)device->data;
  return rt_extern_free(hyper->alloc, (void *)chunk, size);
}

void mc_hyperram_read_async(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, struct fc_task *task)
{
  rt_hyperram_t *hyper = (rt_hyperram_t *)device->data;
  task->done = 0;
  __mc_hyper_copy(UDMA_CHANNEL_ID(hyper->channel) + 0, (uint32_t)addr, hyper_addr, size, task, REG_MBR0);
}

void mc_hyperram_read(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size)
{
  struct fc_task task;
  mc_hyperram_read_async(device, addr, hyper_addr, size, mc_task(&task));
  mc_task_wait(&task);
}

void mc_hyperram_write_async(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, struct fc_task *task)
{
  rt_hyperram_t *hyper = (rt_hyperram_t *)device->data;
  task->done = 0;
  __mc_hyper_copy(UDMA_CHANNEL_ID(hyper->channel) + 1, (uint32_t)addr, hyper_addr, size, task, REG_MBR0);
}

void mc_hyperram_write(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size)
{
  struct fc_task task;
  mc_hyperram_write_async(device, addr, hyper_addr, size, mc_task(&task));
  mc_task_wait(&task);
}

static RT_BOOT_CODE void __attribute__((constructor)) __rt_hyper_init()
{
  __rt_hyper_end_task = NULL;
  __rt_hyper_pending_tasks = NULL;
  __rt_hyper_pending_emu_channel = -1;
}