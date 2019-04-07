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

#include "rt/rt_api.h"

#define MRAM_CMD_TRIM_CFG    0x01
#define MRAM_CMD_NORMAL_TX   0x02
#define MRAM_CMD_ERASE_CHIP  0x04
#define MRAM_CMD_ERASE_SECT  0x08
#define MRAM_CMD_ERASE_WORD  0x10
#define MRAM_CMD_PWDN        0x20
#define MRAM_CMD_READ_RX     0x40
#define MRAM_CMD_REF_LINE_P  0x80
#define MRAM_CMD_REF_LINE_AP 0xC0


#define UDMA_BASE_ADDR 0x1A102000
#define APB_SOC_CTRL_BASE_ADDR  0x1A104000

#define UDMA_MRAM_ID 14
#define PLP_SOC_EVENT_MRAM_TX         4*UDMA_MRAM_ID
#define PLP_SOC_EVENT_MRAM_RX         4*UDMA_MRAM_ID+1
#define PLP_SOC_EVENT_MRAM_ERASE      4*UDMA_MRAM_ID+2
#define PLP_SOC_EVENT_MRAM_PROGRAM    4*UDMA_MRAM_ID+3
#define PLP_SOC_EVENT_MRAM_TRIM_CFG   4*UDMA_MRAM_ID+3
#define PLP_SOC_EVENT_MRAM_REF_LINE   4*UDMA_MRAM_ID+2


#define UDMA_MRAM_BASE_ADDR UDMA_BASE_ADDR + 0x80*(UDMA_MRAM_ID+1)

#define MRAM_REG_RX_SADDR    UDMA_MRAM_BASE_ADDR + 0x00
#define MRAM_REG_RX_SIZE     UDMA_MRAM_BASE_ADDR + 0x04
#define MRAM_REG_RX_CFG      UDMA_MRAM_BASE_ADDR + 0x08
#define MRAM_REG_RX_INTCFG   UDMA_MRAM_BASE_ADDR + 0x0C
#define MRAM_REG_TX_SADDR    UDMA_MRAM_BASE_ADDR + 0x10
#define MRAM_REG_TX_SIZE     UDMA_MRAM_BASE_ADDR + 0x14
#define MRAM_REG_TX_CFG      UDMA_MRAM_BASE_ADDR + 0x18
#define MRAM_REG_TX_INTCFG   UDMA_MRAM_BASE_ADDR + 0x1C
#define MRAM_REG_TX_DADDR    UDMA_MRAM_BASE_ADDR + 0x20
#define MRAM_REG_RX_DADDR    UDMA_MRAM_BASE_ADDR + 0x24
#define MRAM_REG_STATUS      UDMA_MRAM_BASE_ADDR + 0x28
#define MRAM_REG_MODE        UDMA_MRAM_BASE_ADDR + 0x2C
#define MRAM_REG_ERASE_ADDR  UDMA_MRAM_BASE_ADDR + 0x30
#define MRAM_REG_ERASE_SIZE  UDMA_MRAM_BASE_ADDR + 0x34
#define MRAM_REG_CLOCKDIV    UDMA_MRAM_BASE_ADDR + 0x38
#define MRAM_REG_TRIGGER     UDMA_MRAM_BASE_ADDR + 0x3C
#define MRAM_REG_ISR         UDMA_MRAM_BASE_ADDR + 0x40
#define MRAM_REG_IER         UDMA_MRAM_BASE_ADDR + 0x44
#define MRAM_REG_ICR         UDMA_MRAM_BASE_ADDR + 0x48

#define UDMA_CTRL_BASE_ADDR UDMA_BASE_ADDR + 0x100*15


#define SECTOR_ERASE 0
#define NUM_PULSE    1


RT_L2_DATA int unsigned L2_buf_TRIM[133];


static inline void __rt_mram_read_exec(rt_mram_t *mram, void *data, void *addr, size_t size)
{
  unsigned int periph_base = mram->periph_base;

  udma_mram_mram_mode_t mode = { .raw = udma_mram_mram_mode_get(periph_base) };
  mode.cmd = MRAM_CMD_READ_RX;
  udma_mram_mram_mode_set(periph_base, mode.raw);

  udma_mram_rx_daddr_set(periph_base, (((unsigned int)addr) + ARCHI_MRAM_ADDR) >> 3);

  plp_udma_enqueue(periph_base + UDMA_CHANNEL_RX_OFFSET, (int)data, size, UDMA_CHANNEL_CFG_EN);
}



static inline void __rt_mram_program_exec(rt_mram_t *mram, void *data, void *addr, size_t size)
{
  unsigned int periph_base = mram->periph_base;

  udma_mram_mram_mode_t mode = { .raw = udma_mram_mram_mode_get(periph_base) };
  mode.cmd = MRAM_CMD_NORMAL_TX;
  udma_mram_mram_mode_set(periph_base, mode.raw);

  udma_mram_tx_daddr_set(periph_base, (((unsigned int)addr) + ARCHI_MRAM_ADDR) >> 3);

  plp_udma_enqueue(periph_base + UDMA_CHANNEL_TX_OFFSET, (int)data, size, UDMA_CHANNEL_CFG_EN);
}



static inline void __rt_mram_erase_exec(rt_mram_t *mram, void *addr, size_t size)
{
  unsigned int periph_base = mram->periph_base;

  udma_mram_mram_mode_t mode = { .raw = udma_mram_mram_mode_get(periph_base) };
  mode.cmd = MRAM_CMD_ERASE_WORD;
  udma_mram_mram_mode_set(periph_base, mode.raw);

  udma_mram_erase_addr_set(periph_base, (((unsigned int)addr) + ARCHI_MRAM_ADDR) >> 3);
  udma_mram_erase_size_set(periph_base, (size >> 3) - 1);
  udma_mram_trigger_set(periph_base, 0x1);
}



static inline void __rt_mram_erase_sector_exec(rt_mram_t *mram, void *addr)
{
  unsigned int periph_base = mram->periph_base;

  udma_mram_mram_mode_t mode = { .raw = udma_mram_mram_mode_get(periph_base) };
  mode.cmd = MRAM_CMD_ERASE_SECT;
  udma_mram_mram_mode_set(periph_base, mode.raw);

  udma_mram_erase_addr_set(periph_base, (((unsigned int)addr) + ARCHI_MRAM_ADDR) >> 3);
  udma_mram_trigger_set(periph_base, 0x1);
}



static inline void __rt_mram_erase_chip_exec(rt_mram_t *mram)
{
  unsigned int periph_base = mram->periph_base;

  udma_mram_mram_mode_t mode = { .raw = udma_mram_mram_mode_get(periph_base) };
  mode.cmd = MRAM_CMD_ERASE_CHIP;
  udma_mram_mram_mode_set(periph_base, mode.raw);

  udma_mram_trigger_set(periph_base, 0x1);
}



static void __rt_mram_free(rt_mram_t *mram)
{
  if (mram != NULL) {
    rt_free(RT_ALLOC_FC_DATA, (void *)mram, sizeof(rt_mram_t));
  }
}


void irq_ENABLE_MRAM(int mask) {
    pulp_write32(MRAM_REG_IER, mask);
}


void start_TRIM_CFG_MRAM(unsigned int l2_addr, unsigned int mram_addr, unsigned int size_bytes) {
    volatile int tmp = pulp_read32(MRAM_REG_MODE);
    tmp = (tmp & 0xFFFF00FF) | 1 << 8;
    pulp_write32(MRAM_REG_MODE,tmp);
    pulp_write32(MRAM_REG_TX_SADDR,l2_addr);
    pulp_write32(MRAM_REG_TX_DADDR,mram_addr>>3);
    pulp_write32(MRAM_REG_TX_SIZE,size_bytes);
    pulp_write32(MRAM_REG_TX_CFG,0x10);
}


static void __rt_mram_do_trim(rt_mram_t *mram)
{
  rt_event_t *event = rt_event_get_blocking(NULL);

  // Init the CFG zone in L2 to scan the TRIM CGF in the MRAM
  int unsigned sec_option;
  sec_option = (SECTOR_ERASE << 24) | ( NUM_PULSE << 17 );
  for(int index = 0; index < 133; index ++)
  {
      L2_buf_TRIM[index] = 0x00000000u;
  }
  // write the info to num Pulses and Sector enable
  L2_buf_TRIM[123] = sec_option;

  // section erase_en = cr_lat[3960]
  // prog_pulse_cfg   = cr_lat[3955:3953]
  //                    cr_lat[3955:3953]= 000 => number of program pulse = 8 (the default)
  //                    cr_lat[3955:3953]= 001 => number of program pulse = 1
  //                    cr_lat[3955:3953]= 010 => number of program pulse = 2
  //                    cr_lat[3955:3953]= 011 => number of program pulse = 3
  //                    cr_lat[3955:3953]= 100 => number of program pulse = 4
  //                    cr_lat[3955:3953]= 101 => number of program pulse = 5
  //                    cr_lat[3955:3953]= 110 => number of program pulse = 6
  //                    cr_lat[3955:3953]= 111 => number of program pulse = 7


  rt_periph_copy_t *copy = &event->copy;
  mram->first_pending_copy = copy;
  mram->last_pending_copy = copy;
  copy->next = NULL;
  copy->event = event;

  start_TRIM_CFG_MRAM((unsigned int) &(L2_buf_TRIM[0]) , 0x1D000000, 532);



  /* Wait TRIM CFG DONE */

  rt_event_wait(event);
}

static void __rt_mram_do_ref_line(rt_mram_t *mram)
{
  rt_event_t *event = rt_event_get_blocking(NULL);

  rt_periph_copy_t *copy = &event->copy;
  mram->first_pending_copy = copy;
  mram->last_pending_copy = copy;
  copy->next = NULL;
  copy->event = event;

  // Setting  AREF and TMEN to 1
  pulp_write32(MRAM_REG_MODE,0x000080EC);
  pulp_write32(MRAM_REG_TRIGGER,0x1);
  /* Wait REF LINE done */
  rt_event_wait(event);

  event = rt_event_get_blocking(NULL);
  copy = &event->copy;
  mram->first_pending_copy = copy;
  mram->last_pending_copy = copy;
  copy->next = NULL;
  copy->event = event;

  pulp_write32(MRAM_REG_MODE,0x0000C0EC);
  pulp_write32(MRAM_REG_TRIGGER,0x1);

  rt_event_wait(event);


  pulp_write32(MRAM_REG_MODE,0x000000E0);
  rt_time_wait_us(8);

}

static void __rt_mram_init(rt_mram_t *mram)
{
  // Enable ClockDivider [8]  and set CLKDIV=1 --> [7:0]
  pulp_write32(MRAM_REG_CLOCKDIV,0x00000102);

  // Enable MRAM Erase, REF_LINE events, disable Program and TRIM_CFG
  irq_ENABLE_MRAM(0xF);


  // Set PORb, RETb, RSTb, NVR, TMEN, AREF, DPD, ECCBYPS to 0
  pulp_write32(MRAM_REG_MODE,0x00000000);

  // Perform Setup sequence : POR-RET-RST
  pulp_write32(MRAM_REG_MODE,0x00000080);
  pulp_write32(MRAM_REG_MODE,0x000000C0);

  rt_time_wait_us(5);
  pulp_write32(MRAM_REG_MODE,0x000000E0);


  __rt_mram_do_trim(mram);

  __rt_mram_do_ref_line(mram);
}



static rt_flash_t *__rt_mram_open(rt_dev_t *dev, rt_flash_conf_t *conf, rt_event_t *event)
{
  rt_mram_t *mram = NULL;

  mram = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_mram_t));
  if (mram == NULL) goto error;

  mram->periph_id = ARCHI_UDMA_MRAM_ID(conf->id);
  mram->periph_base = hal_udma_periph_base(mram->periph_id);

  mram->first_pending_copy = NULL;

  __rt_udma_callback[mram->periph_id] = __rt_mram_handle_event;
  __rt_udma_callback_data[mram->periph_id] = mram;

  soc_eu_fcEventMask_setEvent(UDMA_EVENT_ID(mram->periph_id));
  soc_eu_fcEventMask_setEvent(UDMA_EVENT_ID(mram->periph_id)+2);
  soc_eu_fcEventMask_setEvent(UDMA_EVENT_ID(mram->periph_id)+3);
  plp_udma_cg_set(plp_udma_cg_get() | (1<<mram->periph_id));

  rt_pm_domain_state_switch(RT_PM_DOMAIN_MRAM, RT_PM_DOMAIN_STATE_ON, NULL);

  __rt_mram_init(mram);

  return (rt_flash_t *)mram;

error:
  __rt_mram_free(mram);
  return NULL;
}



static void __rt_mram_close(rt_flash_t *flash, rt_event_t *event)
{
  rt_mram_t *mram = (rt_mram_t *)flash;
  __rt_mram_free(mram);
}



static void __rt_mram_read(rt_flash_t *flash, void *data, void *addr, size_t size, rt_event_t *event)
{
  rt_mram_t *mram = (rt_mram_t *)flash;

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  if (mram->first_pending_copy == NULL)
  {
    mram->first_pending_copy = copy;
    mram->last_pending_copy = copy;
    copy->next = NULL;
    copy->event = call_event;
    __rt_mram_read_exec((rt_mram_t *)flash, data, addr, size);
  }
  else
  {
    // TODO multiple asynchronous copies is not yet supprted
  }

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}



static void __rt_mram_program(rt_flash_t *flash, void *data, void *addr, size_t size, rt_event_t *event)
{
  rt_mram_t *mram = (rt_mram_t *)flash;

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  if (mram->first_pending_copy == NULL)
  {
    mram->first_pending_copy = copy;
    mram->last_pending_copy = copy;
    copy->next = NULL;
    copy->event = call_event;
    __rt_mram_program_exec((rt_mram_t *)flash, data, addr, size);
  }
  else
  {
    // TODO multiple asynchronous copies is not yet supprted
  }

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}



static void __rt_mram_erase_chip(rt_flash_t *flash, rt_event_t *event)
{
  rt_mram_t *mram = (rt_mram_t *)flash;

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  if (mram->first_pending_copy == NULL)
  {
    mram->first_pending_copy = copy;
    mram->last_pending_copy = copy;
    copy->next = NULL;
    copy->event = call_event;
    __rt_mram_erase_chip_exec((rt_mram_t *)flash);
  }
  else
  {
    // TODO multiple asynchronous copies is not yet supprted
  }

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}



static void __rt_mram_erase_sector(rt_flash_t *flash, void *addr, rt_event_t *event)
{
  rt_mram_t *mram = (rt_mram_t *)flash;

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  if (mram->first_pending_copy == NULL)
  {
    mram->first_pending_copy = copy;
    mram->last_pending_copy = copy;
    copy->next = NULL;
    copy->event = call_event;
    __rt_mram_erase_sector_exec((rt_mram_t *)flash, addr);
  }
  else
  {
    // TODO multiple asynchronous copies is not yet supprted
  }

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}



static void __rt_mram_erase(rt_flash_t *flash, void *addr, int size, rt_event_t *event)
{
  rt_mram_t *mram = (rt_mram_t *)flash;

  int irq = rt_irq_disable();

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_t *copy = &call_event->copy;

  if (mram->first_pending_copy == NULL)
  {
    mram->first_pending_copy = copy;
    mram->last_pending_copy = copy;
    copy->next = NULL;
    copy->event = call_event;
    __rt_mram_erase_exec((rt_mram_t *)flash, addr, size);
  }
  else
  {
    // TODO multiple asynchronous copies is not yet supprted
  }

  __rt_wait_event_check(event, call_event);

  rt_irq_restore(irq);
}



rt_flash_dev_t mram_desc = {
  .open         = &__rt_mram_open,
  .close        = &__rt_mram_close,
  .read         = &__rt_mram_read,
  .program      = &__rt_mram_program,
  .erase_chip   = &__rt_mram_erase_chip,
  .erase_sector = &__rt_mram_erase_sector,
  .erase = &__rt_mram_erase
};
