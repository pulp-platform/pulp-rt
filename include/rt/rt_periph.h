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
 * Copyright (C) 2018 GreenWaves Technologies
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

#ifndef __RT_RT_PERIPH_H__
#define __RT_RT_PERIPH_H__

/// @cond IMPLEM

#define RT_PERIPH_COPY_L2   0

#if PULP_CHIP == CHIP_DEVCHIP || PULP_CHIP == CHIP_PULP || PULP_CHIP == CHIP_WOLFE || PULP_CHIP == CHIP_QUENTIN || PULP_CHIP == CHIP_PULPINO || PULP_CHIP == CHIP_VIVOSOC3
#define RT_PERIPH_SPIM0_RX   0
#define RT_PERIPH_SPIM0_TX   1
#define RT_PERIPH_SPIM1_RX   2
#define RT_PERIPH_SPIM1_TX   3
#define RT_PERIPH_UART_RX    6
#define RT_PERIPH_UART_TX    7
#define RT_PERIPH_I2C0_DATA  8
#define RT_PERIPH_I2C0_CMD   9
#define RT_PERIPH_I2C1_DATA  10
#define RT_PERIPH_I2C1_CMD   11
#define RT_PERIPH_I2S0       12
#define RT_PERIPH_I2S1       13
#define RT_PERIPH_CAM        14
#elif PULP_CHIP == CHIP_PULPISSIMO
#define RT_PERIPH_SPIM0_RX   0
#define RT_PERIPH_SPIM0_TX   1
#define RT_PERIPH_SPIM1_RX   2
#define RT_PERIPH_SPIM1_TX   3
#define RT_PERIPH_UART_RX    6
#define RT_PERIPH_UART_TX    7
#define RT_PERIPH_I2C0_DATA  8
#define RT_PERIPH_I2C0_CMD   9
#define RT_PERIPH_I2C1_DATA  10
#define RT_PERIPH_I2C1_CMD   11
#define RT_PERIPH_I2S0       12
#define RT_PERIPH_I2S1       13
#define RT_PERIPH_CAM        14
#elif PULP_CHIP == CHIP_GAP
#define RT_PERIPH_LVDS0_RX   0
#define RT_PERIPH_LVDS0_TX   1
#define RT_PERIPH_SPIM0_RX   2
#define RT_PERIPH_SPIM0_TX   3
#define RT_PERIPH_SPIM1_RX   4
#define RT_PERIPH_SPIM1_TX   5
#define RT_PERIPH_UART_RX    8
#define RT_PERIPH_UART_TX    9
#define RT_PERIPH_I2C0_DATA  10
#define RT_PERIPH_I2C0_CMD   11
#define RT_PERIPH_I2C1_DATA  12
#define RT_PERIPH_I2C1_CMD   13
#define RT_PERIPH_I2S0       14
#define RT_PERIPH_I2S1       15
#define RT_PERIPH_CAM        16
#endif

static inline void rt_periph_copy_init(rt_periph_copy_t *copy, int flags);

void rt_periph_copy(rt_periph_copy_t *copy, int channel, unsigned int addr, int size,
  unsigned int cfg, rt_event_t *event);

void rt_periph_dual_copy_safe(rt_periph_copy_t *copy, int rx_channel_id, unsigned int tx_addr, int tx_size, unsigned int rx_addr, int rx_size,
  unsigned int cfg);

static inline void rt_periph_dual_copy(rt_periph_copy_t *copy, int rx_channel_id,
  unsigned int tx_addr, int tx_size, unsigned int rx_addr, int rx_size,
  unsigned int cfg, rt_event_t *event)
{
  copy->event = event;
  int irq = hal_irq_disable();
  rt_periph_dual_copy_safe(copy, rx_channel_id, tx_addr, tx_size, rx_addr, rx_size, cfg);
  hal_irq_restore(irq);
}

void __rt_periph_dual_copy_noshadow_safe(rt_periph_copy_t *copy, int rx_channel_id, unsigned int tx_addr, int tx_size, unsigned int rx_addr, int rx_size,
  unsigned int cfg);

static inline void rt_periph_dual_copy_noshadow_safe(rt_periph_copy_t *copy, int rx_channel_id, unsigned int tx_addr, int tx_size, unsigned int rx_addr, int rx_size,
  unsigned int cfg, rt_event_t *event)
{
  copy->event = event;
  __rt_periph_dual_copy_noshadow_safe(copy, rx_channel_id, tx_addr, tx_size,
    rx_addr, rx_size, cfg);
}

static inline void rt_periph_dual_copy_noshadow(rt_periph_copy_t *copy, int rx_channel_id,
  unsigned int tx_addr, int tx_size, unsigned int rx_addr, int rx_size,
  unsigned int cfg, rt_event_t *event)
{
  int irq = hal_irq_disable();
  rt_periph_dual_copy_noshadow_safe(copy, rx_channel_id, tx_addr, tx_size,
    rx_addr, rx_size, cfg, event);
  hal_irq_restore(irq);
}


static inline __attribute__((always_inline)) void __rt_channel_push(rt_periph_channel_t *channel, rt_periph_copy_t *copy)
{
  copy->next = NULL;
  if (likely(channel->first == NULL)) {
    channel->first = copy;
  } else {
    channel->last->next = copy;
  }
  channel->last = copy;
}

static inline __attribute__((always_inline)) void __rt_channel_enqueue(rt_periph_channel_t *channel, rt_periph_copy_t *copy, int addr, int size, int cfg)
{
  copy->addr = addr;
  copy->size = size;
  copy->cfg = cfg;
  if (channel->firstToEnqueue == NULL) channel->firstToEnqueue = copy;
}


static inline void rt_periph_copy_init(rt_periph_copy_t *copy, int flags)
{
  copy->ctrl = 0;
  copy->u.hyper.repeat = 0;
}

static inline void rt_periph_copy_init_callback(rt_periph_copy_t *copy, unsigned int callback)
{
  copy->ctrl = 0;
  copy->u.hyper.repeat = 0;
  copy->enqueue_callback = callback;
}

static inline void rt_periph_copy_init_ctrl(rt_periph_copy_t *copy, int ctrl)
{
  copy->ctrl = ctrl;
  copy->u.hyper.repeat = 0;
}

extern rt_periph_channel_t periph_channels[];

static inline rt_periph_channel_t *__rt_periph_channel(int channel) {
  return &periph_channels[channel];
}

void __rt_periph_wait_event(int event, int clear);

void __rt_periph_clear_event(int event);

/// @endcond

#endif
