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
 * Authors: Yao ZHANG, GreenWaves Technologies (yao.zhang@greenwaves-technologies.com)
 */

#include "rt/rt_api.h"

RT_L2_DATA static _sccb_config_t _sccbConf= {
    I2C_CMD_CFG, 0x04, 0x00 // FLLCLK * 256 (0x100 clkdiv) = 20ns * 256 =
};

RT_L2_DATA static _sccb_short_read_t _short_readReg= {
    I2C_CMD_START,
    I2C_CMD_WR,  0x43,       // Send the control byte: change to read mode
    I2C_CMD_RD_NACK,         // Read NACK, fin of read
    I2C_CMD_STOP,
    I2C_CMD_WAIT,0xff
};

RT_L2_DATA static _sccb_read_t _sccbReadReg= {
    I2C_CMD_START,
    I2C_CMD_WR,  0x42,       // Send the control byte: CS & write mode
    I2C_CMD_WR,  0x00,       // Addr Low Byte
    I2C_CMD_STOP,
    I2C_CMD_START,
    I2C_CMD_WR,  0x43,       // Send the control byte: change to read mode
    I2C_CMD_RD_NACK,         // Read NACK, fin of read
    I2C_CMD_STOP,
    I2C_CMD_WAIT,0xff
};

RT_L2_DATA static _sccb_write_t _sccbWriteReg= {
    I2C_CMD_START,
    I2C_CMD_WR,  0x42,      // Send the control byte: CS & mode write
    I2C_CMD_WR,  0x00,      // Addr Low Byte
    I2C_CMD_WR,  0x00,      // Write Data
    I2C_CMD_STOP,
    I2C_CMD_WAIT,0xff,
};

void rt_sccb_write(rt_i2c_t *dev_sccb, unsigned int addr, unsigned char value, rt_event_t *event){
  int irq = hal_irq_disable();

  _sccbWriteReg.addr_cs_w = dev_sccb->i2c_conf.addr_cs;
  _sccbWriteReg.addr_LSB = (addr & 0xFF);
  _sccbWriteReg.data = value;
  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_sccb->channel) + 1 , (unsigned int) &_sccbWriteReg, sizeof(_sccb_write_t), UDMA_CHANNEL_CFG_EN, call_event);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

void rt_sccb_read(rt_i2c_t *dev_sccb, unsigned int addr, unsigned char *rxBuff, rt_event_t *event){
  int irq = hal_irq_disable();

  _sccbReadReg.addr_cs_w = dev_sccb->i2c_conf.addr_cs;
  _sccbReadReg.addr_cs_r = (dev_sccb->i2c_conf.addr_cs|0x01);
  _sccbReadReg.addr_LSB = (addr & 0xFF);
  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_sccb->channel) + 0 , (unsigned int ) rxBuff, 1,  UDMA_CHANNEL_CFG_EN, call_event);
  // Like here is api for read, so we will have this event ONLY when the read is finished
  rt_periph_copy(&dev_sccb->i2c_conf.writeCopy, UDMA_CHANNEL_ID(dev_sccb->channel) + 1 , (unsigned int)&_sccbReadReg, sizeof(_sccb_read_t),  UDMA_CHANNEL_CFG_EN, 0);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

void sccb_short_read(rt_i2c_t *dev_sccb, unsigned int addr, unsigned char *rxBuff, rt_event_t *event){
  int irq = hal_irq_disable();
  _short_readReg.addr_cs_r = (dev_sccb->i2c_conf.addr_cs|0x01);

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);

  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_sccb->channel) + 0 , (unsigned int ) rxBuff, 1,  UDMA_CHANNEL_CFG_EN, call_event);
  rt_periph_copy(&dev_sccb->i2c_conf.writeCopy, UDMA_CHANNEL_ID(dev_sccb->channel) + 1 , (unsigned int)&_short_readReg, sizeof(_sccb_short_read_t),  UDMA_CHANNEL_CFG_EN, 0);
  hal_irq_restore(irq);
  __rt_wait_event_check(event, call_event);
}

void rt_sccb_conf(rt_i2c_t *dev_sccb, unsigned char addr_cs, unsigned int clk_divider, rt_event_t *event){
  int irq = hal_irq_disable();
  unsigned char sccb_id;
  if (dev_sccb->channel == (RT_PERIPH_I2C0_DATA>>1)) sccb_id = 0;
  else sccb_id = 1;

  plp_udma_cg_set(plp_udma_cg_get() | (1<<ARCHI_UDMA_I2C_ID(sccb_id)));
  if (sccb_id){
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(1)*2);
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(1)*2+1);
  }else{
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(0)*2);
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(0)*2+1);
  }
  dev_sccb->i2c_conf.i2c_id = sccb_id;
  dev_sccb->i2c_conf.addr_cs = addr_cs;
  dev_sccb->i2c_conf.clk_divider = clk_divider;
  _sccbConf.value_MSB = (clk_divider >> 8) & 0xFF;
  _sccbConf.value_LSB = clk_divider & 0xFF;

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_sccb->channel) + 1 , (unsigned int) &_sccbConf, sizeof(_sccb_config_t), UDMA_CHANNEL_CFG_EN, call_event);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

rt_i2c_t *__rt_sccb_open_channel(int channel, rt_i2c_conf_t *sccb_conf, rt_event_t *event){
  rt_i2c_t *sccb = NULL;
  sccb = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_i2c_t));
  if (sccb == NULL) goto error;

  sccb->dev = NULL;
  sccb->channel = channel;
  memcpy(&sccb->i2c_conf, sccb_conf, sizeof(rt_i2c_conf_t));
  rt_periph_copy_init(&sccb->i2c_conf.writeCopy, 0);

  return sccb;

error:
  rt_free(RT_ALLOC_FC_DATA, (void *) sccb, sizeof(rt_i2c_t));
  return NULL;
}

rt_i2c_t *rt_sccb_open(char *dev_name, rt_i2c_conf_t *sccb_conf, rt_event_t *event){
  rt_i2c_t *sccb = NULL;
  rt_dev_t *dev = rt_dev_get(dev_name);
  if (dev == NULL) goto error;
  sccb = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_i2c_t));
  if (sccb == NULL) goto error;

  sccb->dev = dev;
  sccb->channel = dev->channel;
  memcpy(&sccb->i2c_conf, sccb_conf, sizeof(rt_i2c_conf_t));
  rt_periph_copy_init(&sccb_conf->writeCopy, 0);
  return sccb;

error:
  rt_free(RT_ALLOC_FC_DATA, (void *) sccb, sizeof(rt_i2c_t));
  return NULL;
}

void rt_sccb_close (rt_i2c_t *dev_sccb, rt_event_t *event){
  if (dev_sccb != NULL) {
    rt_free(RT_ALLOC_FC_DATA, (void *)dev_sccb, sizeof(rt_i2c_t));
  }
}

