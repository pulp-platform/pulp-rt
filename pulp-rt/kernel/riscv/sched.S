/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
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

#include "rt/rt_data.h"
#include "archi/pulp.h"

#if RISCV_VERSION >= 4

  .global __rt_event_enqueue
__rt_event_enqueue:
  // Can be called with following registers:
  //   x9/s1:  return address
  //   x10/a0: temporary register
  //   x11/a1: the event
  //   x12/a2: temporary register

  // First check if it is a normal event
//  andi    x10, x11, 0x3
//  bne     x10, x0, __rt_handle_special_event

  // Enqueue normal event
  lw      x10, RT_EVENT_T_SCHED(x11)
  sw      x0, RT_EVENT_T_NEXT(x11)
  lw      x12, RT_SCHED_T_FIRST(x10)
  beqz    x12, __rt_no_first
  lw      x12, RT_SCHED_T_LAST(x10)
  sw      x11, RT_EVENT_T_NEXT(x12)
  j       __rt_common

__rt_no_first:
  sw      x11, RT_SCHED_T_FIRST(x10)

__rt_common:
  sw      x11, RT_SCHED_T_LAST(x10)

  // Check if a thread must be waken-up
  lw      x12, RT_SCHED_T_WAITING(x10)
  sw      x0,  RT_SCHED_T_WAITING(x10)
  beqz    x12, enqueue_end

  la      x10, __rt_thread_current
  lw      x10, 0(x10)
  bne     x10, x12, thread_enqueue

enqueue_end:
  jr          x9

thread_enqueue:
  jr          x9

__rt_handle_special_event:
  j __rt_handle_special_event


#if defined(ARCHI_HAS_CLUSTER)
    // This interrupt handler is triggered by cluster for pushing
    // remotly an event
    // The event is temporarly put into a single per-cluster entry
    // The FC must get it and push it to the scheduler

    .global __rt_remote_enqueue_event
__rt_remote_enqueue_event:

    sw  s0, -4(sp)
    sw  s1, -8(sp)
    sw  a0, -12(sp)
    sw  a1, -16(sp)
    sw  a2, -20(sp)

    la   s0, __rt_nb_cluster
    la   s1, __rt_fc_cluster_data
    lw   s1, 0(s1)

    // Loop over the clusters to see if there is an event to push
__rt_remote_enqueue_event_loop_cluster:
    lw   a1, RT_FC_CLUSTER_DATA_T_EVENTS(s1)
    beq  a1, x0, __rt_remote_enqueue_event_loop_cluster_continue

    lw   a2, RT_FC_CLUSTER_DATA_T_TRIG_ADDR(s1)
    sw   x0, RT_FC_CLUSTER_DATA_T_EVENTS(s1)

    sw   x0, 0(a2)

    la   s1, __rt_remote_enqueue_event_loop_cluster_continue
    j    __rt_event_enqueue

__rt_remote_enqueue_event_loop_cluster_continue:
    addi s0, s0, -1
    bgt  s0, x0, __rt_remote_enqueue_event_loop_next_cluster



    lw  s0, -4(sp)
    lw  s1, -8(sp)
    lw  a0, -12(sp)
    lw  a1, -16(sp)
    lw  a2, -20(sp)

    mret

__rt_remote_enqueue_event_loop_next_cluster:
    la   s1, __rt_fc_cluster_data
    lw   s1, 0(s1)
    li   a1, RT_FC_CLUSTER_DATA_T_SIZEOF
    mul  a1, s0, a1
    add  s1, s1, a1
    j __rt_remote_enqueue_event_loop_cluster

#endif

#endif
