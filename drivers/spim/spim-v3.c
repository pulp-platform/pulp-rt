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
 *          Jie Chen, GreenWaves Technologies (jie.chen@greenwaves-technologies.com)
 */

#include "rt/rt_api.h"
#include <stdint.h>

static int __rt_spim_open_count[ARCHI_UDMA_NB_SPIM];

typedef struct {
    unsigned int cmd[4];
} rt_spim_cmd_t;

static inline int __rt_spim_id(int periph_id)
{
  return periph_id - ARCHI_UDMA_SPIM_ID(0);
}

static int __rt_spi_get_div(int spi_freq)
{
  int periph_freq = __rt_freq_periph_get();

  if (spi_freq >= periph_freq)
  {
    return 0;
  }
  else
  {
    // Round-up the divider to obtain an SPI frequency which is below the maximum
    int div = (__rt_freq_periph_get() + spi_freq - 1)/ spi_freq;

    // The SPIM always divide by 2 once we activate the divider, thus increase by 1
    // in case it is even to not go avove the max frequency.
    if (div & 1) div += 1;
    div >>= 1;

    return div;
  }
}

static inline int __rt_spim_get_byte_align(int wordsize, int big_endian)
{
  return wordsize == RT_SPIM_WORDSIZE_32 && big_endian;
}

rt_spim_t *rt_spim_open(char *dev_name, rt_spim_conf_t *conf, rt_event_t *event)
{
  int irq = rt_irq_disable();

  __rt_padframe_init();

  rt_spim_conf_t def_conf;

  if (conf == NULL)
  {
    conf = &def_conf;
    rt_spim_conf_init(conf);
  }

  int channel = -1;

  if (conf->id != -1)
  {  
    rt_trace(RT_TRACE_DEV_CTRL, "[SPIM] Opening spim device (id: %d)\n", conf->id);
    channel = ARCHI_UDMA_SPIM_ID(conf->id);
  }
  else if (dev_name != NULL)
  {
    rt_trace(RT_TRACE_DEV_CTRL, "[SPIM] Opening spim device (name: %s)\n", dev_name);
  
    rt_dev_t *dev = rt_dev_get(dev_name);
    if (dev == NULL) goto error;

    channel = dev->channel;
  }

  if (channel == -1) goto error;

  rt_spim_t *spim = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_spim_t));
  if (spim == NULL) goto error;

  spim->channel = channel*2;

  spim->wordsize = conf->wordsize;
  spim->big_endian = conf->big_endian;
  spim->polarity = conf->polarity;
  spim->phase = conf->phase;
  spim->max_baudrate = conf->max_baudrate;
  spim->cs = conf->cs;
  spim->byte_align = __rt_spim_get_byte_align(conf->wordsize, conf->big_endian);

  int div = __rt_spi_get_div(spim->max_baudrate);
  spim->div = div;

  spim->cfg = SPI_CMD_CFG(div, conf->polarity, conf->phase);

  int id = channel - ARCHI_UDMA_SPIM_ID(0);

  __rt_spim_open_count[id]++;

  if (__rt_spim_open_count[id] == 1)
  {
    plp_udma_cg_set(plp_udma_cg_get() | (1<<channel));

    soc_eu_fcEventMask_setEvent(UDMA_EVENT_ID(channel));
    soc_eu_fcEventMask_setEvent(UDMA_EVENT_ID(channel) + 1);
  }

  rt_irq_restore(irq);

  return spim;

error:
  rt_warning("[SPIM] Failed to open spim device\n");
  return NULL;
}

void __rt_spim_control(rt_spim_t *handle, rt_spim_control_e cmd, uint32_t arg)
{
  int irq = rt_irq_disable();

  int polarity = (cmd >> __RT_SPIM_CTRL_CPOL_BIT) & 3;
  int phase = (cmd >> __RT_SPIM_CTRL_CPHA_BIT) & 3;
  int set_freq = (cmd >> __RT_SPIM_CTRL_SET_MAX_BAUDRATE_BIT) & 1;
  int wordsize = (cmd >> __RT_SPIM_CTRL_WORDSIZE_BIT) & 3;
  int big_endian = (cmd >> __RT_SPIM_CTRL_ENDIANNESS_BIT) & 3;

  if (set_freq)
  {
    handle->max_baudrate = arg;
    handle->div = __rt_spi_get_div(arg);
  }

  if (polarity) handle->polarity = polarity >> 1;
  if (phase) handle->phase = phase >> 1;
  if (wordsize) handle->wordsize = wordsize >> 1;
  if (big_endian) handle->big_endian = big_endian >> 1;


  handle->cfg = SPI_CMD_CFG(handle->div, handle->polarity, handle->phase);
  handle->byte_align = __rt_spim_get_byte_align(handle->wordsize, handle->big_endian);

  rt_irq_restore(irq);
}

void rt_spim_close(rt_spim_t *handle, rt_event_t *event)
{
  int irq = rt_irq_disable();

  int channel = handle->channel >> 1;
  int id = channel - ARCHI_UDMA_SPIM_ID(0);

  __rt_spim_open_count[id]--;

  if (__rt_spim_open_count[id] == 0)
  {
    plp_udma_cg_set(plp_udma_cg_get() & ~(1<<(handle->channel>>1)));

    soc_eu_fcEventMask_clearEvent(handle->channel);
    soc_eu_fcEventMask_clearEvent(handle->channel + 1);
  }

  rt_free(RT_ALLOC_FC_DATA, handle, sizeof(handle));

  rt_irq_restore(irq);
}

static inline void __rt_spim_copy(rt_periph_copy_t *copy, int channel_id, unsigned int buffer_addr, int buffer_size, unsigned int cmd_addr, int cmd_size,
  unsigned int cfg)
{
  // Dual copies assumes that more copies are always pushed to the TX channels than
  // to the RX channel, as an RX copy always needs a header in the TX channel.
  // Thus only the TX channel is checked to know if the transfer can be enqueued
  // directly.

  // The copy is enqueued to the RX channel so that the event is notified when the receive
  // is done.
  rt_periph_channel_t *channel = __rt_periph_channel(channel_id);
  unsigned int periph_base = hal_udma_periph_base(channel_id >> 1);
  unsigned int cmd_base = periph_base + ARCHI_SPIM_CMD_OFFSET;
  unsigned int channel_base = periph_base + (channel_id & 1) * UDMA_CHANNEL_SIZE;

  cfg |= UDMA_CHANNEL_CFG_EN;
  copy->size = buffer_size;

  __rt_channel_push(channel, copy);

  // If less than 2 transfers are enqueued in the channel, we can directly enqueue it
  // Otherwise enqueue it in the SW queue, it will be handled later on by the interrupt handler
  // We have to check if there is no transfer already waiting as since we masked interrupts, the
  // UDMA could have finished one transfer and we want to keep the transfers in-order
  if (likely(!channel->firstToEnqueue && plp_udma_canEnqueue(cmd_base)))
  {
    plp_udma_enqueue(cmd_base, cmd_addr, cmd_size, cfg);
    plp_udma_enqueue(channel_base, buffer_addr, buffer_size, cfg);
  } else {
    //copy->u.dual.rx_addr = rx_addr;
    //copy->u.dual.rx_size = rx_size;
    //copy->enqueue_callback = 0;
    //__rt_channel_enqueue(channel, copy, tx_addr, tx_size, cfg);
  }

}
void __rt_spim_send(rt_spim_t *handle, void *data, size_t len, int qspi, rt_spim_cs_e cs_mode, rt_event_t *event)
{
  rt_trace(RT_TRACE_SPIM, "[SPIM] Send bitstream (handle: %p, buffer: %p, len: 0x%x, qspi: %d, keep_cs: %d, event: %p)\n", handle, data, len, qspi, cs_mode, event);

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  rt_periph_copy_init(copy, 0);
  copy->event = call_event;

  rt_spim_cmd_t *cmd = (rt_spim_cmd_t *)copy->periph_data;
  unsigned int *udma_cmd = (unsigned int *)cmd->cmd;
  *udma_cmd++ = handle->cfg;
  *udma_cmd++ = SPI_CMD_SOT(handle->cs);
  *udma_cmd++ = SPI_CMD_TX_DATA(len/32, SPI_CMD_1_WORD_PER_TRANSF, 32, qspi, SPI_CMD_MSB_FIRST);
  *udma_cmd++ = SPI_CMD_EOT(0);

  __rt_spim_copy(copy, handle->channel + 1, (int)data, len/8, (int)cmd, 4*4, UDMA_CHANNEL_CFG_SIZE_32);

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}

void __rt_spim_receive(rt_spim_t *handle, void *data, size_t len, int qspi, rt_spim_cs_e cs_mode, rt_event_t *event)
{
  rt_trace(RT_TRACE_SPIM, "[SPIM] Receive bitstream (handle: %p, buffer: %p, len: 0x%x, qspi: %d, keep_cs: %d, event: %p)\n", handle, data, len, qspi, cs_mode, event);

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  rt_periph_copy_init(copy, 0);
  copy->event = call_event;

  rt_spim_cmd_t *cmd = (rt_spim_cmd_t *)copy->periph_data;
  unsigned int *udma_cmd = (unsigned int *)cmd->cmd;
  *udma_cmd++ = handle->cfg;
  *udma_cmd++ = SPI_CMD_SOT(handle->cs);
  *udma_cmd++ = SPI_CMD_RX_DATA(len/32, SPI_CMD_1_WORD_PER_TRANSF, 32, qspi, SPI_CMD_MSB_FIRST);
  *udma_cmd++ = SPI_CMD_EOT(0);

  __rt_spim_copy(copy, handle->channel, (int)data, len/8, (int)cmd, 4*4, UDMA_CHANNEL_CFG_SIZE_32);

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}

void rt_spim_transfer(rt_spim_t *handle, void *tx_data, void *rx_data, size_t len, rt_spim_cs_e mode, rt_event_t *event)
{
  rt_trace(RT_TRACE_SPIM, "[SPIM] Transfering bitstream (handle: %p, tx_buffer: %p, rx_buffer: %p, len: 0x%x, keep_cs: %d, event: %p)\n", handle, tx_data, rx_data, len, mode, event);

}

void rt_spim_conf_init(rt_spim_conf_t *conf)
{
  conf->wordsize = RT_SPIM_WORDSIZE_8;
  conf->big_endian = 0;
  conf->max_baudrate = 10000000;
  conf->cs_gpio = -1;
  conf->cs = -1;
  conf->id = -1;
  conf->polarity = 0;
  conf->phase = 0;
}

RT_FC_BOOT_CODE void __attribute__((constructor)) __rt_spim_init()
{
  for (int i=0; i<ARCHI_UDMA_NB_SPIM; i++)
  {
    __rt_spim_open_count[i] = 0;
  }
}
