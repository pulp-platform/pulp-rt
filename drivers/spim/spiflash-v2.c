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

/* 
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 *          Jie Chen, GreenWaves Technologies (jie.chen@greenwaves-technologies.com)
 */

#include "rt/rt_api.h"

static void __rt_spiflash_free(rt_spiflash_t *flash)
{
  if (flash != NULL) {
    rt_free(RT_ALLOC_FC_DATA, (void *)flash, sizeof(rt_spiflash_t));
  }
}

RT_L2_DATA static unsigned int cmd_flashId[] = {
  SPI_CMD_CFG      (8, 0, 0),
  SPI_CMD_SOT      (0),
  SPI_CMD_SEND_CMD (0x06, 8, 0),    //write enable 
  SPI_CMD_EOT      (0),
  SPI_CMD_SOT      (0),           
  SPI_CMD_SEND_CMD (0x0071, 8, 0),  //qpi enable:  CR2V[6] = 1
  SPI_CMD_SEND_ADDR(32, 0),                
  0x800003cf,
  SPI_CMD_EOT      (0)
};

typedef struct {
  unsigned int sot;
  unsigned int sendCmd;
  unsigned int sendAddr;
  unsigned int addr;
  unsigned int sendMode;
  unsigned int dummy;
  unsigned int rxData;
  unsigned int eot;
} flashRead_t;

typedef struct {
  union {
    struct {
      unsigned int sot0;
      unsigned int sendCmd0;
      unsigned int eot0;
      unsigned int sot1;
      unsigned int sendCmd1;
      unsigned int sendAddr;
      unsigned int addr;
      unsigned int eot1;
    } erase;
    struct {
      unsigned int sot;
      unsigned int sendCmd;
      unsigned int rxData;
      unsigned int eot;
    } rdsr2v;
    struct {
      unsigned int sot0;
      unsigned int sendCmd0;
      unsigned int eot0;
      unsigned int sot1;
      unsigned int sendCmd1;
      unsigned int sendAddr;
      unsigned int addr;
      unsigned int txData;
    } page_program_head;
    struct {
      unsigned int eot;
    } page_program_tail;
  };

} flash_cmd_t;

RT_L2_DATA flashRead_t flash_read;
RT_L2_DATA flash_cmd_t flash_cmd;
volatile RT_L2_DATA int cmd_buff;

static rt_flash_t *__rt_spiflash_open(rt_dev_t *dev, rt_flash_conf_t *conf, rt_event_t *event)
{
  rt_spiflash_t *flash = NULL;

  flash = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_spiflash_t));
  if (flash == NULL) goto error;

  int periph_id;

  if (dev)
  {
    periph_id = dev->channel;
  }
  else
  {
    periph_id = ARCHI_UDMA_SPIM_ID(conf->id);
  }
  
  int channel_id = periph_id*2;

  flash->channel = channel_id;

  plp_udma_cg_set(plp_udma_cg_get() | (1<<(periph_id)));

  soc_eu_fcEventMask_setEvent(channel_id);
  soc_eu_fcEventMask_setEvent(channel_id+1);

  flash_read.sot      = SPI_CMD_SOT       (0);
  flash_read.sendCmd  = SPI_CMD_SEND_CMD  (0x03, 8, 0);
  flash_read.sendAddr = SPI_CMD_SEND_ADDR (24, 0);
  flash_read.addr     = 0x00000000;
  flash_read.rxData   = SPI_CMD_RX_DATA   (1, 1, SPI_CMD_BYTE_ALIGN_ENA);
  flash_read.eot      = SPI_CMD_EOT       (0);

  //rt_periph_copy(NULL, channel_id+1, (unsigned int)&cmd_flashId, sizeof(cmd_flashId), 2<<1, event);

  if (event) __rt_event_enqueue(event);

  return (rt_flash_t *)flash;

error:
  __rt_spiflash_free(flash);
  return NULL;
}

static void __rt_spiflash_close(rt_flash_t *flash, rt_event_t *event)
{
  rt_spiflash_t *spi_flash = (rt_spiflash_t *)flash;
  __rt_spiflash_free(spi_flash);
}

void __rt_spiflash_enqueue_callback();

static void __rt_spiflash_read(rt_flash_t *_dev, void *data, void *addr, size_t size, rt_event_t *event)
{
  rt_trace(RT_TRACE_FLASH, "[UDMA] Enqueueing SPI flash read (dev: %p, data: %p, addr: %p, size 0x%x, event: %p)\n", _dev, data, addr, size, event);

  int irq = rt_irq_disable();

  rt_spiflash_t *flash = (rt_spiflash_t *)_dev;

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;
  int channel_id = flash->channel;

  rt_periph_copy_init_ctrl(copy, RT_PERIPH_COPY_SPIFLASH);

  rt_periph_channel_t *channel = __rt_periph_channel(channel_id);
  unsigned int periph_base = hal_udma_periph_base(channel_id >> 1);
  unsigned int tx_base = periph_base + UDMA_CHANNEL_TX_OFFSET;
  unsigned int rx_base = periph_base + UDMA_CHANNEL_RX_OFFSET;
  unsigned int tx_addr = (unsigned int)&flash_read;
  unsigned int rx_addr = (int)data;
  int tx_size = sizeof(flashRead_t);
  int rx_size = size;
  unsigned int flash_addr_cmd = SPI_CMD_SEND_ADDR_VALUE(((int)addr)<<8);
  unsigned int flash_size_cmd = SPI_CMD_RX_DATA(size*8, 0, SPI_CMD_BYTE_ALIGN_ENA);

  unsigned int cfg = (2<<1) | UDMA_CHANNEL_CFG_EN;
  copy->event = call_event;

  __rt_channel_push(channel, copy);

  if (likely(!channel->firstToEnqueue && !plp_udma_busy(tx_base)))
  {
    flash_read.addr = flash_addr_cmd;
    flash_read.rxData = flash_size_cmd;

    plp_udma_enqueue(rx_base, rx_addr, rx_size, cfg);
    plp_udma_enqueue(tx_base, tx_addr, tx_size, cfg);
  } else {
    copy->size = tx_size;
    copy->cfg = cfg;
    copy->u.raw.val[0] = rx_addr;
    copy->u.raw.val[1] = rx_size;
    copy->u.raw.val[2] = flash_addr_cmd;
    copy->u.raw.val[3] = flash_addr_cmd;
    __rt_channel_enqueue(channel, copy, tx_addr, tx_size, cfg);
  }

__rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}

static void __rt_spiflash_program(rt_flash_t *_dev, void *data, void *addr, size_t size, rt_event_t *event)
{
  int irq = rt_irq_disable();

  rt_spiflash_t *flash = (rt_spiflash_t *)_dev;

  flash_cmd.page_program_head.sot0     = SPI_CMD_SOT       (0),
  flash_cmd.page_program_head.sendCmd0 = SPI_CMD_SEND_CMD  (0x06, 8, 0),    //write enable 
  flash_cmd.page_program_head.eot0     = SPI_CMD_EOT       (0),
  flash_cmd.page_program_head.sot1     = SPI_CMD_SOT       (0);
  flash_cmd.page_program_head.sendCmd1 = SPI_CMD_SEND_CMD  (0x02, 8, 0);
  flash_cmd.page_program_head.sendAddr = SPI_CMD_SEND_ADDR (24, 0);
  flash_cmd.page_program_head.addr     = ((int)addr)<<8;
  flash_cmd.page_program_head.txData   = SPI_CMD_TX_DATA   (size*8, 0, SPI_CMD_BYTE_ALIGN_ENA);
  rt_periph_copy(NULL, flash->channel+1, (unsigned int)&flash_cmd, sizeof(flash_cmd.page_program_head), 2<<1, NULL);

  rt_periph_copy(NULL, flash->channel+1, (unsigned int)data, size, 2<<1, NULL);

  flash_cmd.page_program_tail.eot     = SPI_CMD_EOT       (0);
  rt_periph_copy(NULL, flash->channel+1, (unsigned int)&flash_cmd, sizeof(flash_cmd.page_program_tail), 2<<1, NULL);

  rt_irq_restore(irq);
}

static uint32_t __rt_spiflash_read_sr2v(rt_spiflash_t *flash)
{
  flash_cmd.rdsr2v.sot      = SPI_CMD_SOT       (0);
  flash_cmd.rdsr2v.sendCmd  = SPI_CMD_SEND_CMD  (0x07, 8, 0);
  flash_cmd.rdsr2v.rxData   = SPI_CMD_RX_DATA   (8, 0, SPI_CMD_BYTE_ALIGN_DIS);
  flash_cmd.rdsr2v.eot      = SPI_CMD_EOT       (0);

  cmd_buff = 0;

  rt_event_t *event = __rt_wait_event_prepare_blocking();
  rt_periph_copy_t *copy = &event->copy;
  rt_periph_copy_init(copy, 0);
  rt_periph_dual_copy_safe(copy, flash->channel, (unsigned int)&flash_cmd, sizeof(flash_cmd.rdsr2v), (int)&cmd_buff, 1, 0<<1);
  __rt_wait_event(event);

  return cmd_buff;
}

static void __rt_spiflash_erase_sector(rt_flash_t *_dev, void *data, rt_event_t *event)
{
  int irq = rt_irq_disable();

  rt_spiflash_t *flash = (rt_spiflash_t *)_dev;

  flash_cmd.erase.sot0     = SPI_CMD_SOT       (0),
  flash_cmd.erase.sendCmd0 = SPI_CMD_SEND_CMD  (0x06, 8, 0),    //write enable 
  flash_cmd.erase.eot0     = SPI_CMD_EOT       (0),
  flash_cmd.erase.sot1     = SPI_CMD_SOT       (0);
  flash_cmd.erase.sendCmd1 = SPI_CMD_SEND_CMD  (0xD8, 8, 0);
  flash_cmd.erase.sendAddr = SPI_CMD_SEND_ADDR (24, 0);
  flash_cmd.erase.addr     = ((int)data)<<8;
  flash_cmd.erase.eot1     = SPI_CMD_EOT       (0);

  rt_periph_copy(NULL, flash->channel+1, (unsigned int)&flash_cmd, sizeof(flash_cmd.erase), 2<<1, NULL);

  while (((__rt_spiflash_read_sr2v(flash) >> 2) & 1) == 0)
  {
    rt_time_wait_us(100);
  }

  rt_irq_restore(irq);
}

static void __rt_spiflash_erase_chip(rt_flash_t *_dev, rt_event_t *event)
{
  printf("%s %d\n", __FILE__, __LINE__);
}

rt_flash_dev_t spiflash_desc = {
  .open         = &__rt_spiflash_open,
  .close        = &__rt_spiflash_close,
  .read         = &__rt_spiflash_read,
  .program      = &__rt_spiflash_program,
  .erase_chip   = &__rt_spiflash_erase_chip,
  .erase_sector = &__rt_spiflash_erase_sector
};
