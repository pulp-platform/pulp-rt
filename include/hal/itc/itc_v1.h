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

#ifndef __HAL_ITC_ITC_V1_H__
#define __HAL_ITC_ITC_V1_H__

#include "hal/pulp.h" 
#include "hal/pulp_io.h" 
#include "archi/itc/itc_v1.h"


static inline void hal_itc_enable_value_set(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK, mask);
}

static inline unsigned int hal_itc_enable_value_get() {
  return pulp_read32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK);
}

static inline void hal_itc_enable_set(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK_SET, mask);
}

static inline void hal_itc_enable_clr(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_MASK_CLR, mask);
}


static inline void hal_itc_ack_value_set(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_ACK, mask);
}

static inline unsigned int hal_itc_ack_value_get() {
  return pulp_read32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_ACK);
}

static inline void hal_itc_ack_set(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_ACK_SET, mask);
}

static inline void hal_itc_ack_clr(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_ACK_CLR, mask);
}



static inline void hal_itc_status_value_set(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_STATUS, mask);
}

static inline void hal_itc_status_value_set_remote(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_STATUS, mask);
}

static inline unsigned int hal_itc_status_value_get() {
  return pulp_read32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_STATUS);
}

static inline void hal_itc_status_set(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_STATUS_SET, mask);
}

static inline void hal_itc_status_clr(unsigned int mask) {
  pulp_write32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_STATUS_CLR, mask);
}


static inline unsigned int hal_itc_fifo_pop() {
  return pulp_read32(ARCHI_FC_ITC_ADDR + ARCHI_ITC_FIFO);
}


static inline void hal_itc_wait_for_interrupt() {
  asm volatile ("wfi");
}


static inline void hal_itc_wait_for_event_noirq(unsigned int mask) {
  hal_itc_enable_set(mask);

  int end = 0;
  do {
    unsigned int state = hal_spr_read_then_clr(0x300, 0x1<<3);
    if ((hal_itc_status_value_get() & mask) == 0) {
      asm volatile ("wfi");
    } else {
      end = 1;
    }
    hal_spr_write(0x300, state);
  } while (!end);
  hal_itc_status_clr(mask);
  hal_itc_enable_clr(mask);
}


static inline void hal_itc_wait_for_event(unsigned int mask) {
  hal_itc_enable_set(mask);

  int end = 0;
  do {
    unsigned int state = hal_spr_read_then_clr(0x300, 0x1<<3);
    if ((hal_itc_ack_value_get() & mask) == 0) {
      asm volatile ("wfi");
    } else {
      end = 1;
    }
    hal_spr_write(0x300, state);
  } while (!end);
  hal_itc_ack_clr(mask);
}



#endif
