/*
 * Copyright (C) 2019 GreenWaves Technologies
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
 * Authors: Germain Haugou, GreenWaves Technologies (germain.haugou@greenwaves-technologies.com)
 */

#ifdef __RT_USE_PROFILE
#include "archi/gvsoc/gvsoc.h"
#endif

#include "data.h"
#include "archi/pulp.h"
#include "archi/itc/itc_v1.h"
#include "archi/eu/eu_v3.h"
#include "archi/cluster_ctrl/cluster_ctrl_v2.h"


    .section .cluster.text , "ax"

  .global __rt_pe_start
__rt_pe_start:


    // CC saved registers
    //   s0:  cluster_call_pool
    //   s1:  
    //   s2:  core id
    //   s3:  event unit base
    //   s4:  sleep event
    //   s5:  call return address
    //   s6:  task completion callback
    //   s7:  fc cluster data
    //   s8:  fc enqueue event
    //   s9:  fc event unit base
    //   s10: 
    //   s11: 

    // Worker saved registers
    //   s0: sleep event
    //   s1: barrier base
    //   s2: core id
    //   s3: event unit base
    //   s4: fork return address
    //   s5: fork return address
    //   s6: task
    //   s7: slave task entry
    //   s8: slave done
    //   s9: cc trigger base
    //   s10: 
    //   s11: dispatcher base

    //
    // COMMON DEFINITIONS BETWEEN CC AND WORKERS
    //

    // Core ID
    csrr    a0, 0xF14
    andi    s2, a0, 0x1f
    srli    a0, a0, 5

    // Activate a few events
    li      t0, (1<<PULP_DISPATCH_EVENT) | (1<<PULP_HW_BAR_EVENT) | (1<<PULP_MUTEX_EVENT)
    li      t1, ARCHI_EU_DEMUX_ADDR
    sw      t0, EU_CORE_MASK(t1)



    li      t2, ARCHI_CC_CORE_ID
    bne     s2, t2, __rt_worker_start

    //
    // CC DEFINITIONS
    //

    // Cluster task pool
    la      s0, __rt_cluster_pool

    // Init allocators here as we don t have C code on cluster side to init them
    li      t2,     (1 << ARCHI_CLUSTER_NB_PE) - 1
    sw      t2, %tiny(__rt_free_core)(x0)
    li      t2,     (1 << (1)) - 1
    sw      t2, %tiny(__rt_free_hw_sync)(x0)
    sw      x0, %tiny(__rt_slave_done_global)(x0)

    // Prepare few values that will be kept in saved registers to optimize the loop
    li      s3, ARCHI_EU_DEMUX_ADDR
    li      s4, 1<<RT_CLUSTER_CALL_EVT
    la      s5, __rt_cc_call_return
    la      s7, __rt_fc_cluster_data
    li      t2, RT_FC_CLUSTER_DATA_T_SIZEOF
    mul     t2, t2, a0
    add     s7, s7, t2
    addi    s7, s7, RT_FC_CLUSTER_DATA_T_EVENTS
    li      s9, ARCHI_FC_ITC_ADDR + ITC_STATUS_SET_OFFSET
    li      s8, 1<<RT_FC_ENQUEUE_EVENT

    j       __rt_cc_call_loop



    //
    // CC LOOP
    //

    // We get back there after a cluster call has finished execution
    // Notif the FC in case there is an event and proceed with the next call
__rt_cc_call_return:
    beq     s6, x0, __rt_cc_call_loop



__rt_cc_call_notif_fc_retry:
    // Now we have to push the termination event to FC side
    // First wait until the slot for posting events is free
    lw      t0, 0(s7)
    bne     t0, x0, __rt_cc_call_notif_fc_wait

    // Push it
    sw      s6, 0(s7)

    // And notify the FC side with a HW event to execute the associated handler
    sw      s8, 0(s9)


    // Main loop on CC where we wait until a cluster task is pushed
__rt_cc_call_loop:

    // Check if a task is ready in the pool or a worker has finished a task
    lw      t5, %tiny(__rt_slave_done_global)(x0)
    lw      t3, RT_CLUSTER_CALL_POOL_T_FIRST_CALL_FC_FOR_CL(s0)

    // Check if a call is ready, e.g. if nb_pe is not zero
    // otherwise go to sleep
    bnez    t5, __rt_cc_offload_end_handler
    beq     t3, x0, __rt_cc_call_wait



__rt_cc_call_loop_update_next:
    lw      t4, RT_CLUSTER_TASK_NEXT(t3)
    sw      x0, RT_CLUSTER_TASK_PENDING(t3)
    sw      t4, RT_CLUSTER_CALL_POOL_T_FIRST_CALL_FC_FOR_CL(s0)

    // Check again next pointer in case it was updated by the FC.
    // If so, do it it again as this will ensure that either we see the new
    // value or the FC sees our write
    lw      t5, RT_CLUSTER_TASK_NEXT(t3)
    bne     t4, t5, __rt_cc_call_loop_update_next


__rt_cc_call_exec:

#ifdef __RT_USE_PROFILE
    li      a0, GV_SEMIHOSTING_VCD_DUMP_TRACE
    lw      a1, %tiny(__rt_pe_trace)(x0)
    li      a2, 0
    li      a4, 0
    ebreak
#endif

#ifdef __RT_USE_ASSERT
    csrwi   0x7D0, 0
#endif

    // Reads entry point information
    lw      a0, RT_CLUSTER_TASK_ARG(t3)
    lw      t0, RT_CLUSTER_TASK_ENTRY(t3)
    lw      sp, RT_CLUSTER_TASK_STACKS(t3)
    lw      t1, RT_CLUSTER_TASK_STACK_SIZE(t3)
    lw      t2, RT_CLUSTER_TASK_SLAVE_STACK_SIZE(t3)
    lw      t5, RT_CLUSTER_TASK_CORE_MASK(t3)
    lw      s6, RT_CLUSTER_TASK_COMPLETION_CALLBACK(t3)
    lw      t6, RT_CLUSTER_TASK_NB_CORES(t3)
    lw      a1, RT_CLUSTER_TASK_IS_CC_TASK(t3)
    mv      ra, s5

    add     sp, sp, t1

    beqz    a1, __rt_cc_call_forward_to_pe

#ifdef __RT_USE_ASSERT
    // Update stack checking information
    beqz    t1, __rt_no_stack_check
    sub     t4, sp, t1
    csrw    0x7D1, t4
    csrw    0x7D2, sp
    csrwi   0x7D0, 1
#endif

__rt_no_stack_check:
    
    # Make sure the completion task is not executed
    mv      s6, x0
    
    // Call the entry point, this will directly come back to the master loop
    jr      t0


__rt_cc_call_wait:
#ifdef __RT_USE_PROFILE
    li      a0, GV_SEMIHOSTING_VCD_DUMP_TRACE
    lw      a1, %tiny(__rt_pe_trace)(x0)
    li      a2, 1
    ebreak
#endif

    sw      s4, EU_CORE_MASK_OR(s3)
    p.elw   x0, EU_CORE_EVENT_WAIT_CLEAR(s3)
    sw      s4, EU_CORE_MASK_AND(s3)
    j       __rt_cc_call_loop




__rt_cc_call_notif_fc_wait:
    sw      s4, EU_CORE_MASK_OR(s3)
    p.elw   x0, EU_CORE_EVENT_WAIT_CLEAR(s3)
    sw      s4, EU_CORE_MASK_AND(s3)
    j       __rt_cc_call_notif_fc_retry


__rt_cc_offload_end_handler:
    la        t5, __rt_slave_done
    la        t6, __rt_slave_tasks
    sw        x0, %tiny(__rt_slave_done_global)(x0)
    lp.setupi x1,ARCHI_CLUSTER_NB_PE, __rt_cc_offload_end_handler_loop_end
    p.lb      t4, 0(t5!)
    p.lw      t3, 0(t6!)

    bnez      t4, __rt_cc_offload_end_handler_exec

__rt_cc_offload_end_handler_loop_end:
    nop

    j         __rt_cc_call_loop

__rt_cc_offload_end_handler_exec:
    lw        t3, CL_TASK_CLUSTER_TASK(t3)
    beqz      t3, __rt_cc_call_loop
    j         __rt_cc_call_exec


__rt_cc_call_forward_to_pe:
    sw      t3, %tiny(__rt_cluster_forward_call)(x0)
    la      x12, __rt_cluster_forward_call_to_pe
    la      x9, __rt_cc_call_return
    j       __rt_call_external_c_function




    //
    // WORKER DEFINITIONS
    //

__rt_worker_start:

    // Prepare TLS area
    la      s0, __rt_cluster_pool
    lw      tp, RT_CLUSTER_CALL_POOL_T_TLS_BASE(s0)
    lw      t2, RT_CLUSTER_CALL_POOL_T_TLS_SIZE(s0)
    mul     t2, t2, s2
    add     tp, tp, t2

    li      s11, ARCHI_EU_DEMUX_ADDR + EU_DISPATCH_DEMUX_OFFSET
    sw      s11, %tprel_lo(dispatcher_base)(t4)
    li      s1, ARCHI_EU_DEMUX_ADDR + EU_BARRIER_DEMUX_OFFSET
    sw      s1, %tprel_lo(barrier_base)(t4)
    li      t2, ARCHI_EU_DEMUX_ADDR + EU_MUTEX_DEMUX_OFFSET
    sw      t2, %tprel_lo(mutex_base)(t4)

    
    li      s9, ARCHI_EU_ADDR + EU_SW_EVENTS_AREA_BASE + EU_CORE_TRIGG_SW_EVENT + (RT_CLUSTER_CALL_EVT << 2)


    la      s4, __rt_slave_fork_return
    la      s5, __rt_slave_wait_for_dispatch
    li      s3, ARCHI_EU_DEMUX_ADDR
    li      s0, 1<<RT_CLUSTER_CALL_EVT

    // Compute right address to get slave task depending on core id
    la      s7, __rt_slave_tasks
    slli    t1, s2, 2
    add     s7, s7, t1

    la      s8, __rt_slave_done
    slli    t1, s2, 2
    add     s8, s8, t1

    //j       __rt_slave_wait_for_dispatch



    //
    // WORKER TASK LOOP
    //

    // Both master and slaves worker get here to wait for a task

    .global __rt_worker_wait_task
__rt_worker_wait_task:

    lw      s6, 0(s7)
    beqz    s6, __rt_worker_wait_task_wakeup

    lw      t5, CL_TASK_CORE_MASK(s6)
    lw      t0, CL_TASK_STACKS(s6)
    lw      t1, CL_TASK_STACK_SIZE(s6)
    lw      s11, CL_TASK_DISPATCHER_BASE(s6)
    lw      s1, CL_TASK_BARRIER_BASE(s6)
    lw      t6, CL_TASK_MUTEX_BASE(s6)

    // Find the virtual core ID from the core mask and our real ID
    mv      t2, s2
    beqz    s2, __rt_worker_core_0_vid

    sw      x0, 0(s7)

    addi    t3, s2, -1
    slli    t3, t3, 5
    p.extractur t2, t5, t3 
    p.cnt   t2, t2

__rt_worker_core_0_vid:
    // Set slave stack
    addi    a5, t2, 1
    mul     sp, t1, a5
    add     sp, sp, t0

    // Set HW resources base in TLS area
    sw      s11, %tprel_lo(dispatcher_base)(t4)
    sw      s1, %tprel_lo(barrier_base)(t4)
    sw      t6, %tprel_lo(mutex_base)(t4)

    // Set number of cores in TLS area
    p.cnt   t6, t5
    sw      t6, %tprel_lo(__rt_team_nb_cores)(t4)

    // First core becomes the master, the others the slaves
    bnez    t2, __rt_slave_wait_for_dispatch

__rt_master_loop:
    la      s4, __rt_master_fork_return
    lw      s10, CL_TASK_SLAVE_MASK(s6)
    j       __rt_master_wait_for_dispatch

__rt_master_fork_return:
    // Set the task done
    lw      t5, CL_TASK_CLUSTER_TASK(s6)
    sw      x0, CL_TASK_PENDING(s6)
    // Notify CC that the task is done
    li      t6, 1
    beqz    t5, __rt_master_fork_return_no_notif
    // Global notif
    sw      t6, %tiny(__rt_slave_done_global)(x0)
    // Per core notif
    sw      t6, 0(s8)
__rt_master_fork_return_no_notif:
    // Event trigger to wake it up
    li      t6, 1<<ARCHI_CC_CORE_ID
    sw      t6, 0(s9)


__rt_master_wait_for_dispatch:

    // Wait for PC + arg information from dispatcher
    p.elw   t0, EU_DISPATCH_FIFO_ACCESS(s11)
    p.elw   a0, EU_DISPATCH_FIFO_ACCESS(s11)

    sw      s10, EU_DISPATCH_TEAM_CONFIG(s11)

__rt_master_fork_entry:

    // Jump to the handler and prepare r9 to jump back just before the main loop
    add     ra, s4, x0
    jr      t0



__rt_worker_wait_task_wakeup:
    sw      s0, EU_CORE_MASK_OR(s3)
    p.elw   x0, EU_CORE_EVENT_WAIT_CLEAR(s3)
    sw      s0, EU_CORE_MASK_AND(s3)

    j       __rt_worker_wait_task



__rt_slave_fork_return:

#if 0 //def ARCHI_HAS_CC
    // When the cluster has a controller barrier 0 is used for normal team barrier
    // and barrier 1 is used for end of offload
    p.elw   t0, EU_HW_BARR_TRIGGER_WAIT_CLEAR + EU_BARRIER_SIZE(s1)
#else
    p.elw   t0, EU_HW_BARR_TRIGGER_WAIT_CLEAR(s1)
#endif
    

__rt_slave_wait_for_dispatch:

#ifdef __RT_USE_PROFILE
    li      a0, GV_SEMIHOSTING_VCD_DUMP_TRACE
    slli    t2, s3, 2
    lw      a1, %tiny(__rt_pe_trace)(t2)
    li      a2, 1
    ebreak
#endif

    // Wait for PC + arg information from dispatcher
    p.elw   t0, EU_DISPATCH_FIFO_ACCESS(s11)
    p.elw   a0, EU_DISPATCH_FIFO_ACCESS(s11)

#ifdef __RT_USE_PROFILE
    mv      t1, a0
    li      a0, GV_SEMIHOSTING_VCD_DUMP_TRACE
    slli    t2, s3, 2
    lw      a1, %tiny(__rt_pe_trace)(t2)
    li      a2, 0
    li      a4, 1
    ebreak
    mv      a0, t1
#endif

    // Check if this is an entry with a barrier at the end (fork entry)
    andi    t1, t0, 1
    bne     t1, zero, __rt_other_entry

__rt_fork_entry:

    // Jump to the handler and prepare r9 to jump back just before the main loop
    add     ra, s4, x0
    jr      t0

__rt_other_entry:

  // Jump to the handler and prepare r9 to jump back directly to the main loop
    add     ra, s5, x0
    jr      t0



  .global __rt_slave_set_slave_stack
__rt_slave_set_slave_stack:

#ifdef __RT_USE_ASSERT
    csrwi   0x7D0, 0
#endif

    // Multiply the stack size by the core ID and add the stack base to get our stack
    p.elw   t0, EU_DISPATCH_FIFO_ACCESS(s11)
#ifdef ARCHI_HAS_CC
    // If the cluster has a cluster controller, the first slave has core ID 0
    // and thus we need to take the next stack
    addi     t5, s3, 1
    p.mul   t4, t5, a0
#else
    p.mul   t4, s3, a0
#endif
    add     sp, t4, t0

#ifdef __RT_USE_ASSERT
    beqz    a0, __rt_no_stack_check_end
    sub     t4, sp, a0
    csrw    0x7D1, t4
    csrw    0x7D2, sp
    csrwi   0x7D0, 1
#endif
__rt_no_stack_check_end:
    ret

    .global __rt_worker_deinit
__rt_worker_deinit:
    sw      x0, 0(s7)

    li      t6, 1<<ARCHI_CC_CORE_ID
    sw      t6, 0(s9)

    j __rt_worker_wait_task