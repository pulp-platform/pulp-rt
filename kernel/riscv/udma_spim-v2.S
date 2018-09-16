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
#if UDMA_VERSION == 2
#include "archi/udma/udma_v2.h"
#else
#include "archi/udma/udma_v3.h"
#endif
#if SOC_EU_VERSION == 2
#include "archi/soc_eu/soc_eu_v2.h"
#else
#include "archi/soc_eu/soc_eu_v1.h"
#endif


  // x9: channel, x10: event, x18,x11,x12:temp
__rt_spim_handle_copy:
  
  // Get the current and move the first waiting one to current one
  lw     x11, RT_PERIPH_CHANNEL_T_FIRST_TO_ENQUEUE(x9)
  lw     x8, RT_PERIPH_CHANNEL_T_FIRST(x9)
  sw     x11, RT_PERIPH_CHANNEL_T_FIRST(x9)

  beqz   x11, __rt_spim_no_pending

  lw     x12, RT_PERIPH_COPY_T_NEXT(x11)
  sw     x12, RT_PERIPH_CHANNEL_T_FIRST_TO_ENQUEUE(x9)

  // Check the completion mode
  lw     x12, RT_PERIPH_COPY_T_RAW_VAL5(x11)

  li     x10, 1
  beq    x12, x10, __rt_spim_no_evt_enable


  // Completion with no eot
  // We must activate tx event
  li     x12, 1
  sll    x10, x12, x10
  not    x10, x10
  li     x12, ARCHI_SOC_EU_ADDR
  lw     x12, SOC_FC_MASK_LSB(x12)
  and    x10, x10, x12
  li     x12, ARCHI_SOC_EU_ADDR
  sw     x10, SOC_FC_MASK_LSB(x12)



  // x8: copy, x9:channel, x11: pending copy
__rt_spim_no_evt_enable:

  // Now enqueue the pending copy to the udma
  // First the command buffer
  lw     x12, RT_PERIPH_COPY_T_RAW_VAL0(x11)
  lw     x10, RT_PERIPH_COPY_T_RAW_VAL1(x11)
  ori    x9, x12, UDMA_CHANNEL_TX_OFFSET
  lw     x12, RT_PERIPH_COPY_T_RAW_VAL2(x11)
  sw     x10, UDMA_CHANNEL_SADDR_OFFSET(x9)
  sw     x12, UDMA_CHANNEL_SIZE_OFFSET(x9)
  li     x10 , UDMA_CHANNEL_CFG_EN
  sw     x10, UDMA_CHANNEL_CFG_OFFSET(x9)

  // Then the user buffer
  lw     x9, RT_PERIPH_COPY_T_RAW_VAL0(x11)
  lw     x10, RT_PERIPH_COPY_T_RAW_VAL3(x11)
  lw     x12, RT_PERIPH_COPY_T_RAW_VAL4(x11)
  sw     x10, UDMA_CHANNEL_SADDR_OFFSET(x9)
  sw     x12, UDMA_CHANNEL_SIZE_OFFSET(x9)
  li     x10 , UDMA_CHANNEL_CFG_EN | (2<<1)
  sw     x10, UDMA_CHANNEL_CFG_OFFSET(x9)

  // Check if we have to generate eot
  lw     x12, RT_PERIPH_COPY_T_RAW_VAL5(x11)
  beqz   x12, __rt_spim_no_eot

  // If so we must first wait until udma is ready as we
  // have just pushed 2 transfers
  lw     x11, RT_PERIPH_COPY_T_RAW_VAL1(x11)
  ori    x9, x9, UDMA_CHANNEL_TX_OFFSET
__rt_spim_wait_done:
  lw     x10, UDMA_CHANNEL_CFG_OFFSET(x9)
  andi   x10, x10, UDMA_CHANNEL_CFG_SHADOW
  bnez   x10, __rt_spim_wait_done

  li     x10, SPI_CMD_EOT(1)
  sw     x10, 0(x11)
  sw     x11, UDMA_CHANNEL_SADDR_OFFSET(x9)
  li     x10, 4
  sw     x10, UDMA_CHANNEL_SIZE_OFFSET(x9)
  li     x10, UDMA_CHANNEL_CFG_EN
  sw     x10, UDMA_CHANNEL_CFG_OFFSET(x9)


  // x8: copy
__rt_spim_no_eot:
__rt_spim_no_pending:
  lw     x11, RT_PERIPH_COPY_T_EVENT(x8)

  la     x9, udma_event_handler_end
  bne    x11, zero, __rt_event_enqueue




  // x9: channel, x10:event, x8,x11,x12:temp
  .global __rt_spim_handle_eot
__rt_spim_handle_eot:

  la     x8, periph_channels
  addi   x10, x10, (- ARCHI_SOC_EVENT_SPIM0_EOT + ARCHI_UDMA_SPIM_ID(0))
  slli   x9, x10, RT_PERIPH_CHANNEL_T_SIZEOF_LOG2 + 1
  add    x9, x9, x8

  j      __rt_spim_handle_copy



  .global __rt_spim_handle_tx_end

  // x9: channel, x10:event, x8,x11,x12:temp
__rt_spim_handle_tx_end:

  li     x12, ARCHI_SOC_EU_ADDR
  lw     x11, SOC_FC_MASK_LSB(x12)
  li     x8, 1
  sll    x8, x8, x10
  or     x8, x8, x11
  sw     x8, SOC_FC_MASK_LSB(x12)

  j      __rt_spim_handle_copy
