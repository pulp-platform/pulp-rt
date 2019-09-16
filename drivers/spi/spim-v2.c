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
 * Authors: Germain Haugou, GreenWaves Technologies (germain.haugou@greenwaves-technologies.com)
 */

#include "pmsis.h"
#include <stdint.h>

L2_DATA static pi_spim_t __rt_spim[ARCHI_UDMA_NB_SPIM];

typedef struct 
{
  pi_spim_t *spim;
  uint32_t rx_cmd;
  uint8_t *receive_addr_ucode;
  uint32_t receive_addr_ucode_size;
  uint8_t *send_addr_ucode;
  uint32_t send_addr_ucode_size;
  unsigned int udma_receive_cmd[8];
  unsigned int udma_send_cmd[8];
  uint32_t udma_receive_cmd_size;
  uint32_t udma_send_cmd_size;
  int max_baudrate;
  unsigned int cfg;
  uint32_t max_size;
  char cs;
  char wordsize;
  char big_endian;
  signed char cs_gpio;
  char channel;
  char byte_align;
  unsigned char div;
  char polarity;
  char phase;
} pi_spim_cs_t;

typedef struct {
    unsigned int cmd[4];
} rt_spim_cmd_t;


void __pi_handle_waiting_copy(pi_task_t *task);

void __rt_spi_handle_repeat(void *arg);

static inline void __attribute__((always_inline)) __rt_spi_soc_eu_set(unsigned int event)
{
  soc_eu_eventMask_set(SOC_FC_MASK_LSB, __BITCLR_R(soc_eu_eventMask_get(SOC_FC_MASK_LSB), 1, event));
}

static inline void __attribute__((always_inline)) __rt_spi_soc_eu_clr(unsigned int event)
{
  soc_eu_eventMask_set(SOC_FC_MASK_LSB, __BITSET_R(soc_eu_eventMask_get(SOC_FC_MASK_LSB), 1, event));
}

static void __rt_spi_handle_pending_task(pi_spim_t *spim)
{
  pi_task_t *task = spim->pending_copy;
  spim->pending_copy = NULL;
  __rt_event_handle_end_of_task(task);

  task = spim->waiting_first;
  if (task)
  {
    spim->waiting_first = task->implem.next;
    __pi_handle_waiting_copy(task);
  }
}

#ifndef __RT_SPIM_COPY_ASM

void __pi_spim_handle_eot(int event, void *arg)
{
  pi_spim_t *spim = (pi_spim_t *)arg;

  if (spim->pending_repeat_len)
  {
    __rt_spi_handle_repeat((void *)spim);
    return;
  }

  __rt_spi_handle_pending_task(spim);
}

void __rt_spim_handle_rx_copy(int event, void *arg)
{
  soc_eu_fcEventMask_clearEvent(event);
  __pi_spim_handle_eot(event, arg);
}

void __rt_spim_handle_tx_copy(int event, void *arg)
{
  soc_eu_fcEventMask_clearEvent(event);
  __pi_spim_handle_eot(event, arg);
}

#else

extern void __rt_spim_handle_tx_copy();
extern void __rt_spim_handle_rx_copy();
extern void __pi_spim_handle_eot(void *arg);
extern void __pi_spim_handle_copy(void *arg);
extern void __rt_spim_handle_end_of_copy(void *arg);

#endif



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
    // in case it is even to not go above the max frequency.
    if (div & 1) div += 1;
    div >>= 1;

    return div;
  }
}



static inline int __rt_spim_get_byte_align(int wordsize, int big_endian)
{
  return wordsize == PI_SPI_WORDSIZE_32 && big_endian;
}



static void __rt_spi_apply_conf(pi_spim_cs_t *spim_cs)
{
  spim_cs->udma_receive_cmd[0] = spim_cs->cfg;
  spim_cs->udma_receive_cmd[1] = SPI_CMD_SOT(spim_cs->cs);
  spim_cs->udma_receive_cmd_size = 2;

  spim_cs->udma_send_cmd[0] = spim_cs->cfg;
  spim_cs->udma_send_cmd[1] = SPI_CMD_SOT(spim_cs->cs);
  spim_cs->udma_send_cmd_size = 2;

  spim_cs->rx_cmd = SPI_CMD_RX_DATA(0, 0, spim_cs->byte_align);
}



int pi_spi_open(struct pi_device *device)
{
  int irq = rt_irq_disable();

  struct pi_spi_conf *conf = (struct pi_spi_conf *)device->config;

  int periph_id = ARCHI_UDMA_SPIM_ID(conf->itf);
  int channel_id = UDMA_EVENT_ID(periph_id);

  pi_spim_t *spim = &__rt_spim[conf->itf];

  pi_spim_cs_t *spim_cs = pmsis_l2_malloc(sizeof(pi_spim_cs_t));
  if (spim_cs == NULL) goto error;

  device->data = (void *)spim_cs;

  spim_cs->channel = channel_id;
  spim_cs->spim = spim;
  spim_cs->wordsize = conf->wordsize;
  spim_cs->big_endian = conf->big_endian;
  spim_cs->polarity = conf->polarity;
  spim_cs->phase = conf->phase;
  spim_cs->max_baudrate = conf->max_baudrate;
  spim_cs->cs = conf->cs;
  spim_cs->byte_align = __rt_spim_get_byte_align(conf->wordsize, conf->big_endian);
  spim_cs->max_size = -1;

  int div = __rt_spi_get_div(spim_cs->max_baudrate);
  spim_cs->div = div;

  spim_cs->cfg = SPI_CMD_CFG(div, conf->polarity, conf->phase);

  __rt_spi_apply_conf(spim_cs);

  spim->open_count++;
  if (spim->open_count == 1)
  {
    plp_udma_cg_set(plp_udma_cg_get() | (1<<periph_id));
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_SPIM0_EOT + conf->itf);
    __rt_udma_register_extra_callback(ARCHI_SOC_EVENT_SPIM0_EOT + conf->itf, __pi_spim_handle_eot, (void *)spim);
    __rt_udma_register_channel_callback(channel_id, __rt_spim_handle_rx_copy, (void *)spim);
    __rt_udma_register_channel_callback(channel_id+1, __rt_spim_handle_tx_copy, (void *)spim);

    spim->channel = channel_id;
    spim->pending_repeat_len = 0;
    spim->pending_repeat_callback = 0;
    spim->periph_base = hal_udma_periph_base(periph_id);

    __rt_spi_soc_eu_set(spim->channel);

  }

  rt_irq_restore(irq);

  return 0;

error:
  rt_irq_restore(irq);
  return -1;
}

void pi_spi_ioctl(struct pi_device *device, uint32_t cmd, void *_arg)
{
  int irq = rt_irq_disable();
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  uint32_t arg = (uint32_t)_arg;

  int polarity = (cmd >> __PI_SPI_CTRL_CPOL_BIT) & 3;
  int phase = (cmd >> __PI_SPI_CTRL_CPHA_BIT) & 3;
  int set_freq = (cmd >> __PI_SPI_CTRL_SET_MAX_BAUDRATE_BIT) & 1;
  int wordsize = (cmd >> __PI_SPI_CTRL_WORDSIZE_BIT) & 3;
  int big_endian = (cmd >> __PI_SPI_CTRL_ENDIANNESS_BIT) & 3;

  if (set_freq)
  {
    spim_cs->max_baudrate = arg;
    spim_cs->div = __rt_spi_get_div(arg);
  }

  if (polarity) spim_cs->polarity = polarity >> 1;
  if (phase) spim_cs->phase = phase >> 1;
  if (wordsize) spim_cs->wordsize = wordsize >> 1;
  if (big_endian) spim_cs->big_endian = big_endian >> 1;


  spim_cs->cfg = SPI_CMD_CFG(spim_cs->div, spim_cs->polarity, spim_cs->phase);
  spim_cs->byte_align = __rt_spim_get_byte_align(spim_cs->wordsize, spim_cs->big_endian);

  __rt_spi_apply_conf(spim_cs);

  rt_irq_restore(irq);
}

void pi_spi_close(struct pi_device *device)
{
  int irq = rt_irq_disable();
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;

  int channel = spim_cs->channel;
  int periph_id = UDMA_PERIPH_ID(channel);

  spim->open_count--;

  if (spim->open_count == 0)
  {
    // Deactivate event routing
    soc_eu_fcEventMask_clearEvent(channel);

    // Reactivate clock-gating
    plp_udma_cg_set(plp_udma_cg_get() & ~(1<<periph_id));
  }

  rt_irq_restore(irq);
}



void __rt_spi_handle_repeat_eot(void *arg)
{
  //printf("HANDLE EOT\n");
  pi_spim_t *spim = (pi_spim_t *)arg;

  spim->pending_repeat_callback = 0;

  spim->udma_cmd[0] = SPI_CMD_EOT(1);
  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_TX_OFFSET, (unsigned int)spim->udma_cmd, 1*4, UDMA_CHANNEL_CFG_EN);
}









void pi_spi_send_async(struct pi_device *device, void *data, size_t len, pi_spi_flags_e flags, pi_task_t *task)
{
  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;
  int qspi = ((flags >> 2) & 0x3) == 1;
  int cs_mode = (flags >> 0) & 0x3;

  if (spim->pending_copy)
  {
    task->implem.data[0] = 0;
    task->implem.data[1] = (int)device;
    task->implem.data[2] = (int)data;
    task->implem.data[3] = len;
    task->implem.data[4] = flags;

    if (spim->waiting_first)
      spim->waiting_last->implem.next = task;
    else
      spim->waiting_first = task;

    spim->waiting_last = task;
    task->implem.next = NULL;

    goto end;
  }

  unsigned int base = hal_udma_channel_base(spim_cs->channel + 1);

  if (len > 8192*8)
  {
    spim->pending_repeat_len = len - 8192*8;
    spim->pending_repeat_addr = (uint32_t)data + 8192;
    spim->pending_repeat_base = base;
    spim->pending_repeat_device = device;
    spim->pending_repeat_send = 1;
    spim->pending_repeat_flags = flags;
    //spim->pending_repeat_callback = (int)__rt_spi_handle_send_repeat;
    len = 8192*8;
  }
  
  spim->pending_copy = task;

  int size = (len + 7) >> 3;

  // First enqueue the header with SPI config, cs, and send command.
  // The rest will be sent by the assembly code.
  // First the user data and finally an epilogue with the EOT command.

  int cmd_size = spim_cs->udma_send_cmd_size;
  spim_cs->udma_send_cmd[cmd_size++] = SPI_CMD_TX_DATA(len, qspi, spim_cs->byte_align);

  if (cs_mode == PI_SPI_CS_AUTO && spim->pending_repeat_len == 0)
  {
    // CS auto mode. We handle the termination with an EOT so we have to enqueue
    // 3 transfers.
    // Enqueue fist SOT and user buffer.
    plp_udma_enqueue(base, (unsigned int)spim_cs->udma_send_cmd, cmd_size*4, UDMA_CHANNEL_CFG_EN);
    plp_udma_enqueue(base, (unsigned int)data, size, UDMA_CHANNEL_CFG_EN);

    // Then wait until first one is finished
    while(!plp_udma_canEnqueue(base));

    // And finally enqueue the EOT.
    // The user notification will be sent as soon as the last transfer
    // is done and next pending transfer will be enqueued
    spim->udma_cmd[0] = SPI_CMD_EOT(1);
    plp_udma_enqueue(base, (unsigned int)spim->udma_cmd, 1*4, UDMA_CHANNEL_CFG_EN);
  }
  else
  {
    // CS keep mode.
    // Note this is also used for CS auto mode when we need to repeat the transfer to overcome
    // HW limitation, as CS must be kept low.
    // We cannot use EOT due to HW limitations (generating EOT is always releasing CS)
    // so we have to use TX event instead.
    // TX event is current inactive, enqueue first transfer first EOT.
    plp_udma_enqueue(base, (unsigned int)spim_cs->udma_send_cmd, cmd_size*4, UDMA_CHANNEL_CFG_EN);
    // Then wait until it is finished (should be very quick).
    while(plp_udma_busy(base));
    // Then activateTX event and enqueue user buffer.
    // User notification and next pending transfer will be handled in the handler.
    soc_eu_fcEventMask_setEvent(spim_cs->channel + 1);
    plp_udma_enqueue(base, (unsigned int)data, size, UDMA_CHANNEL_CFG_EN);
  }

end:
  rt_irq_restore(irq);
}



void pi_spi_send(struct pi_device *device, void *data, size_t len, pi_spi_flags_e flags)
{
  pi_task_t task;
  pi_spi_send_async(device, data, len, flags, pi_task_block(&task));
  pi_task_wait_on(&task);
}






static void __pi_spi_receive_handle_prefix_with_aligned_and_suffix_2(void *arg)
{
  pi_spim_t *spim = (pi_spim_t *)arg;
  //printf("HANDLE PREFIX_WITH_ALIGNED 2\n");

  uint32_t size = spim->pending_repeat_misaligned_size;
  uint32_t prefix_addr = spim->pending_repeat_addr;
  uint32_t addr = prefix_addr & ~3;
  uint32_t value = *(uint32_t *)addr;

 //printf("HANDLING PREFIX size %lx addr %lx value %lx new value %x pending len %x\n", size, addr, value, __BITINSERT_R(value, spim->prefix, size*8, (prefix_addr & 3)*8), spim->pending_repeat_len);
  
  value = __BITINSERT_R(value, spim->suffix, size*8, 0);
  *(uint32_t *)addr = value;

  if (!spim->pending_is_auto)
    __rt_spi_handle_pending_task(arg);

  spim->pending_repeat_callback = 0;
}



static void __pi_spi_receive_handle_prefix_with_aligned_and_suffix_1(void *arg)
{
  pi_spim_t *spim = (pi_spim_t *)arg;

  if (spim->pending_is_auto)
      __rt_spi_handle_repeat_eot(arg);

  spim->pending_repeat_callback = (int)__pi_spi_receive_handle_prefix_with_aligned_and_suffix_2;
}



static void __pi_spi_receive_handle_prefix_with_aligned_and_suffix_0(void *arg)
{
  pi_spim_t *spim = (pi_spim_t *)arg;

  uint32_t size = spim->pending_repeat_misaligned_size;
  uint32_t prefix_addr = spim->pending_repeat_misaligned_addr;
  uint32_t addr = prefix_addr & ~3;
  uint32_t value = *(uint32_t *)addr;
  uint32_t suffix_len = spim->pending_repeat_len;

  value = __BITINSERT_R(value, spim->prefix, size*8, (prefix_addr & 3)*8);
  *(uint32_t *)addr = value;

  spim->pending_repeat_addr += size;
  spim->pending_repeat_misaligned_size = (spim->pending_repeat_len + 7) >> 3;

  spim->pending_repeat_callback = (int)__pi_spi_receive_handle_prefix_with_aligned_and_suffix_1;

  spim->udma_cmd[0] = __BITINSERT(spim->rx_cmd, suffix_len-1, SPI_CMD_RX_DATA_SIZE_WIDTH, SPI_CMD_RX_DATA_SIZE_OFFSET);

  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_RX_OFFSET, (unsigned int)&spim->suffix, 4, UDMA_CHANNEL_CFG_EN | (2<<1));
  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_TX_OFFSET, (unsigned int)spim->udma_cmd, 1*4, UDMA_CHANNEL_CFG_EN);
}



static void __attribute__((noinline)) __pi_spi_receive_exec_prefix_with_aligned_and_suffix(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  uint32_t prefix_size = 4 - (addr & 0x3);
  uint32_t prefix_len = prefix_size*8;
  uint32_t aligned_size = (size - prefix_size) & ~0x3;
  uint32_t aligned_len = aligned_size*8;
  uint32_t remaining_len = len - prefix_len;

  if (aligned_len > remaining_len)
    aligned_len = remaining_len;

  spim->pending_repeat_callback = (int)__pi_spi_receive_handle_prefix_with_aligned_and_suffix_0;

  spim->pending_repeat_misaligned_size = prefix_size;
  spim->pending_repeat_len = len - prefix_len - aligned_len;
  spim->pending_repeat_misaligned_addr = addr;
  spim->pending_repeat_addr = addr + prefix_size + aligned_size;

  spim_cs->udma_receive_cmd[cmd_size++] = __BITINSERT(spim->rx_cmd, prefix_len-1, SPI_CMD_RX_DATA_SIZE_WIDTH, SPI_CMD_RX_DATA_SIZE_OFFSET);
  spim_cs->udma_receive_cmd[cmd_size++] = __BITINSERT(spim->rx_cmd, aligned_len-1, SPI_CMD_RX_DATA_SIZE_WIDTH, SPI_CMD_RX_DATA_SIZE_OFFSET);

  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_RX_OFFSET, (unsigned int)&spim->prefix, 4, UDMA_CHANNEL_CFG_EN | (2<<1));
  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_RX_OFFSET, (unsigned int)addr + prefix_size, aligned_size, UDMA_CHANNEL_CFG_EN | (2<<1));
  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_TX_OFFSET, (unsigned int)spim_cs->udma_receive_cmd, cmd_size*4, UDMA_CHANNEL_CFG_EN);
}



static void __attribute__((noinline)) __pi_spi_receive_exec_suffix(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  printf("EXEC SUFFIX\n");
}



static void __attribute__((noinline)) __pi_spi_receive_exec_prefix_with_aligned(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  printf("EXEC PREFIX WITH ALIGNED\n");
}



static void __attribute__((noinline)) __pi_spi_receive_exec_prefix_with_suffix(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  printf("EXEC PREFIX WITH SUFFIX\n");
}



static void __attribute__((noinline)) __pi_spi_receive_exec_aligned(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  spim_cs->udma_receive_cmd[cmd_size++] = __BITINSERT(spim->rx_cmd, len-1, SPI_CMD_RX_DATA_SIZE_WIDTH, SPI_CMD_RX_DATA_SIZE_OFFSET);

  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_RX_OFFSET, (unsigned int)addr, size, UDMA_CHANNEL_CFG_EN | (2<<1));
  plp_udma_enqueue(spim->periph_base + UDMA_CHANNEL_TX_OFFSET, (unsigned int)spim_cs->udma_receive_cmd, cmd_size*4, UDMA_CHANNEL_CFG_EN);

  spim->pending_repeat_callback = (int)0;
  spim->pending_repeat_asm_callback = (int)__rt_spim_handle_end_of_copy;

  if (spim->pending_is_auto)
  {
    spim->pending_repeat_asm_callback = (int)0;
    __rt_spi_handle_repeat_eot((void *)spim);
  }
}



static void __attribute__((noinline)) __pi_spi_receive_exec_aligned_with_suffix(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  printf("EXEC ALIGNED WITH SUFFIX \n");
}



static void __attribute__((noinline)) __pi_spi_receive_exec_aligned_burst(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size, uint32_t cmd_size)
{
  printf("EXEC ALIGNED BURST\n");
}



static void __attribute__((noinline)) __pi_spi_receive_handle_misaligned(pi_spim_cs_t *spim_cs, uint32_t addr, uint32_t len, pi_spim_t *spim, uint32_t size)
{
  if (size < 4)
  {
    __pi_spi_receive_exec_suffix(spim_cs, addr, len, spim, size, spim_cs->udma_receive_cmd_size);
  }
  else if (addr & 0x3)
  {
    uint32_t prefix_size = 4 - (addr & 3);
    uint32_t aligned_size = size - prefix_size;
    uint32_t max_size = spim_cs->max_size;

    if (size > max_size)
    {
      uint32_t burst_size = max_size - 4 + prefix_size;
      __pi_spi_receive_exec_prefix_with_aligned(spim_cs, addr, burst_size*8, spim, burst_size, spim_cs->udma_receive_cmd_size);
    }
    else if (aligned_size < 4)
    {
      __pi_spi_receive_exec_prefix_with_suffix(spim_cs, addr, len, spim, size, spim_cs->udma_receive_cmd_size);
    }
    else
    {
      __pi_spi_receive_exec_prefix_with_aligned_and_suffix(spim_cs, addr, len, spim, size, spim_cs->udma_receive_cmd_size);
    }
  }
  else
  {
    uint32_t max_size = spim_cs->max_size;

    if (size > max_size)
    {
      __pi_spi_receive_exec_aligned_burst(spim_cs, addr, max_size*8, spim, max_size, spim_cs->udma_receive_cmd_size);
    }
    else if (size & 0x3)
    {
      __pi_spi_receive_exec_aligned_with_suffix(spim_cs, addr, len, spim, size, spim_cs->udma_receive_cmd_size);
    }
    else
    {
      __pi_spi_receive_exec_aligned(spim_cs, addr, len, spim, size, spim_cs->udma_receive_cmd_size);
    }
  }
}



static void __attribute__((noinline)) __pi_spi_enqueue_to_pending(pi_spim_t *spim, pi_task_t *task, uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3, uint32_t data4)
{
  task->implem.data[0] = data0;
  task->implem.data[1] = data1;
  task->implem.data[2] = data2;
  task->implem.data[3] = data3;
  task->implem.data[4] = data4;

  if (spim->waiting_first)
    spim->waiting_last->implem.next = task;
  else
    spim->waiting_first = task;

  spim->waiting_last = task;
  task->implem.next = NULL;
}


void pi_spi_receive_async(struct pi_device *device, void *data, size_t len, pi_spi_flags_e flags, pi_task_t *task)
{
  //rt_trace(RT_TRACE_SPIM, "[SPIM] Receive bitstream (handle: %p, buffer: %p, len: 0x%x, qspi: %d, keep_cs: %d, event: %p)\n", handle, data, len, qspi, cs_mode, event);

  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;

  int irq = rt_irq_disable();

  if (likely(!spim->pending_copy))
  {
    uint32_t addr = (uint32_t)data;
    uint32_t size = (len + 7) >> 3;

    int qspi = __BITEXTRACT(flags, 2, 2) == 1;
    int cs_mode = __BITEXTRACT(flags, 2, 0);

    spim->pending_copy = task;
    spim->pending_is_auto = cs_mode == PI_SPI_CS_AUTO;
    spim->rx_cmd = __BITINSERT(spim_cs->rx_cmd, qspi, SPI_CMD_RX_DATA_QPI_WIDTH, SPI_CMD_RX_DATA_QPI_OFFSET);

    __pi_spi_receive_handle_misaligned(spim_cs, addr, len, spim, size);
  }
  else
  {
    __pi_spi_enqueue_to_pending(spim, task, 1, (int)device, (int)data, len, flags);
  }

  rt_irq_restore(irq);
}

void pi_spi_receive(struct pi_device *device, void *data, size_t len, pi_spi_flags_e flags)
{
  pi_task_t task;
  pi_spi_receive_async(device, data, len, flags, pi_task_block(&task));
  pi_task_wait_on(&task);
}


void pi_spi_transfer_async(struct pi_device *device, void *tx_data, void *rx_data, size_t len, pi_spi_flags_e flags, pi_task_t *task)
{
  //rt_trace(RT_TRACE_SPIM, "[SPIM] Transfering bitstream (handle: %p, tx_buffer: %p, rx_buffer: %p, len: 0x%x, flags: 0x%x, event: %p)\n", handle, tx_data, rx_data, len, flags, event);

  int irq = rt_irq_disable();

  __rt_task_init(task);

  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;
  pi_spim_t *spim = spim_cs->spim;
  int cs_mode = (flags >> 0) & 0x3;

  if (spim->pending_copy)
  {
    task->implem.data[0] = 2;
    task->implem.data[1] = (int)device;
    task->implem.data[2] = (int)tx_data;
    task->implem.data[3] = (int)rx_data;
    task->implem.data[4] = len;
    task->implem.data[5] = cs_mode;

    if (spim->waiting_first)
      spim->waiting_last->implem.next = task;
    else
      spim->waiting_first = task;

    spim->waiting_last = task;
    task->implem.next = NULL;

    goto end;
  }

  int channel_id = spim_cs->channel;

  unsigned int rx_base = hal_udma_channel_base(channel_id);
  unsigned int tx_base = hal_udma_channel_base(channel_id+1);

  if (len > 8192*8)
  {
    spim->pending_repeat_len = len - 8192*8;
    spim->pending_repeat_base = tx_base;
    spim->pending_repeat_addr = (uint32_t)rx_data + 8192;
    spim->pending_repeat_dup_addr = (uint32_t)tx_data + 8192;
    spim->pending_repeat_device = device;
    spim->pending_repeat_send = 2;
    spim->pending_repeat_flags = flags;
    //spim->pending_repeat_callback = (int)__rt_spi_handle_transfer_repeat;
    len = 8192*8;
  }
  
  spim->pending_copy = task;

  // First enqueue the header with SPI config, cs, and send command.
  // The rest will be sent by the assembly code.
  // First the user data and finally an epilogue with the EOT command.
  spim->udma_cmd[0] = spim_cs->cfg;
  spim->udma_cmd[1] = SPI_CMD_SOT(spim_cs->cs);
  spim->udma_cmd[2] = SPI_CMD_FUL(len, spim_cs->byte_align);

  int size = (len + 7) >> 3;

  if (cs_mode == PI_SPI_CS_AUTO && spim->pending_repeat_len == 0)
  {
    __rt_spi_soc_eu_clr(spim->channel);
    plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, 3*4, UDMA_CHANNEL_CFG_EN);
    plp_udma_enqueue(rx_base, (unsigned int)rx_data, size, UDMA_CHANNEL_CFG_EN | (2<<1));
    plp_udma_enqueue(tx_base, (unsigned int)tx_data, size, UDMA_CHANNEL_CFG_EN);

    while(!plp_udma_canEnqueue(tx_base));

    spim->udma_cmd[0] = SPI_CMD_EOT(1);
    plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, 1*4, UDMA_CHANNEL_CFG_EN);
  }
  else
  {
    plp_udma_enqueue(tx_base, (unsigned int)spim->udma_cmd, 3*4, UDMA_CHANNEL_CFG_EN);
    while(plp_udma_busy(tx_base));
    __rt_spi_soc_eu_set(spim->channel);
    plp_udma_enqueue(rx_base, (unsigned int)rx_data, size, UDMA_CHANNEL_CFG_EN | (2<<1));
    plp_udma_enqueue(tx_base, (unsigned int)tx_data, size, UDMA_CHANNEL_CFG_EN);
  }

end:
  rt_irq_restore(irq);
}

void pi_spi_transfer(struct pi_device *device, void *tx_data, void *rx_data, size_t len, pi_spi_flags_e flags)
{
  pi_task_t task;
  pi_spi_transfer_async(device, tx_data, rx_data, len, flags, pi_task_block(&task));
  pi_task_wait_on(&task);
}



void *pi_spi_receive_ucode_set(struct pi_device *device, uint8_t *ucode, uint32_t ucode_size)
{
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;

  memcpy(&spim_cs->udma_receive_cmd[2], ucode, ucode_size);
  spim_cs->udma_receive_cmd_size = 2 + (ucode_size >> 2);

  return (void *)&spim_cs->udma_receive_cmd[2];
}



void pi_spi_receive_ucode_set_addr_info(struct pi_device *device, uint8_t *ucode, uint32_t ucode_size)
{
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;

  spim_cs->receive_addr_ucode = ucode;
  spim_cs->receive_addr_ucode_size = ucode_size;
}



void *pi_spi_send_ucode_set(struct pi_device *device, uint8_t *ucode, uint32_t ucode_size)
{
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;

  memcpy(&spim_cs->udma_send_cmd[2], ucode, ucode_size);
  spim_cs->udma_send_cmd_size = 2 + (ucode_size >> 2);

  return (void *)&spim_cs->udma_send_cmd[2];
}



void pi_spi_send_ucode_set_addr_info(struct pi_device *device, uint8_t *ucode, uint32_t ucode_size)
{
  pi_spim_cs_t *spim_cs = (pi_spim_cs_t *)device->data;

  spim_cs->send_addr_ucode = ucode;
  spim_cs->send_addr_ucode_size = ucode_size;
}



void __pi_handle_waiting_copy(pi_task_t *task)
{
  if (task->implem.data[0] == 0)
    pi_spi_send_async((struct pi_device *)task->implem.data[1], (void *)task->implem.data[2], task->implem.data[3], task->implem.data[4], task);
  else if (task->implem.data[0] == 1)
    pi_spi_receive_async((struct pi_device *)task->implem.data[1], (void *)task->implem.data[2], task->implem.data[3], task->implem.data[4], task);
  else
    pi_spi_transfer_async((struct pi_device *)task->implem.data[1], (void *)task->implem.data[2], (void *)task->implem.data[3], task->implem.data[4], task->implem.data[5], task);
}

void pi_spi_conf_init(struct pi_spi_conf *conf)
{
  conf->wordsize = PI_SPI_WORDSIZE_8;
  conf->big_endian = 0;
  conf->max_baudrate = 10000000;
  conf->cs_gpio = -1;
  conf->cs = -1;
  conf->itf = 0;
  conf->polarity = 0;
  conf->phase = 0;
}

static void __attribute__((constructor)) __rt_spim_init()
{
  for (int i=0; i<ARCHI_UDMA_NB_SPIM; i++)
  {
    __rt_spim[i].open_count = 0;
    __rt_spim[i].pending_copy = NULL;
    __rt_spim[i].waiting_first = NULL;
    __rt_spim[i].id = i;
    __rt_udma_channel_reg_data(UDMA_EVENT_ID(ARCHI_UDMA_SPIM_ID(0) + i), &__rt_spim[i]);
    __rt_udma_channel_reg_data(UDMA_EVENT_ID(ARCHI_UDMA_SPIM_ID(0) + i)+1, &__rt_spim[i]);
  }
}

#ifdef __ZEPHYR__

#include <zephyr.h>
#include <device.h>
#include <init.h>

static int spi_init(struct device *device)
{
  ARG_UNUSED(device);

  __rt_spim_init();

  return 0;
}

struct spi_config {
};

struct spi_data {
};

static const struct spi_config spi_cfg = {
};

static struct spi_data spi_data = {
};

DEVICE_INIT(spi, "spi", &spi_init,
    &spi_data, &spi_cfg,
    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif