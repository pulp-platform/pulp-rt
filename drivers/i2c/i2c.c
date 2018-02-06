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

/* A I2C CMD:
 *
 *  I2C_CMD_START,
 *  Address(7bits) + R/W (1bit: 0-W, 1-R)
 *  Address internal (2Bytes)
 *  I2C_CMD_RPT (Optional)
 *  Tx Data or Read Cmd
 *  I2C_CMD_STOP,
 *  I2C_CMD_WAIT
 *
 * example:
 *  I2C_CMD_START,
 *  I2C_CMD_WR,0xA4,  (Addr: 0x52)
 *  I2C_CMD_WR,0x00,
 *  I2C_CMD_WR,0x00,
 *  I2C_CMD_RPT,0x10,
 *  I2C_CMD_WR,0x00,0x01,0x02,0x03,
 *             0x04,0x05,0x06,0x07,
 *             0x08,0x09,0x0A,0x0B,
 *             0x0C,0x0D,0x0E,0x0F,
 *  I2C_CMD_STOP,
 *  I2C_CMD_WAIT,0x10,
 *
 */

static RT_L2_DATA unsigned char _i2c_seq[20];
static RT_L2_DATA int32_t seq_index = 0;

RT_L2_DATA static i2c_config_t _i2cConf = {
  I2C_CMD_CFG, 0x00, 0x00
};

RT_L2_DATA static i2c_read_t _readReg = {
  I2C_CMD_START,
  I2C_CMD_WR,  0xA0,       // Send the control byte: CS & write mode
  I2C_CMD_WR,  0x00,       // Addr High Byte
  I2C_CMD_WR,  0x00,       // Addr Low Byte
  I2C_CMD_START,
  I2C_CMD_WR,  0xA1,       // Send the control byte: change to read mode
  I2C_CMD_RPT, 0x00,       // Repeat the next cmd X times, if X = 0, ignore the next cmd
  I2C_CMD_RD_ACK,          // Read ACK
  I2C_CMD_RD_NACK,         // Read NACK, fin of read
  I2C_CMD_STOP,
  I2C_CMD_WAIT,0x1
};

RT_L2_DATA static i2c_write_t _writeReg = {
  I2C_CMD_START,
  I2C_CMD_WR,  0xA0,      // Send the control byte: CS & mode write
  I2C_CMD_WR,  0x00,      // Addr High Byte
  I2C_CMD_WR,  0x00,      // Addr Low Byte
  I2C_CMD_WR,  0x0,       // Write Data
  I2C_CMD_STOP,
  I2C_CMD_WAIT,0x1
};

void rt_i2c_write_seq(rt_i2c_t *dev_i2c, unsigned char *data, int length, rt_event_t *event){
    int irq = hal_irq_disable();
    _i2c_seq[seq_index++] = I2C_CMD_START;
    _i2c_seq[seq_index++] = I2C_CMD_WR;
    _i2c_seq[seq_index++] = dev_i2c->i2c_conf.addr_cs;
    for (int i=0; i<length; i++){
        _i2c_seq[seq_index++] = I2C_CMD_WR;
        _i2c_seq[seq_index++] = data[i];
    }
    _i2c_seq[seq_index++] = I2C_CMD_STOP;
    _i2c_seq[seq_index++] = I2C_CMD_WAIT;
    _i2c_seq[seq_index++] = 0x1;
    rt_event_t *call_event = __rt_wait_event_prepare(event);
    rt_periph_copy_init(&call_event->copy, 0);
    rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_i2c->channel) + 1 , (unsigned int) &_i2c_seq, seq_index, UDMA_CHANNEL_CFG_EN, call_event);
    __rt_wait_event_check(event, call_event);
    seq_index = 0;
    hal_irq_restore(irq);
}

void rt_i2c_read_seq(rt_i2c_t *dev_i2c, unsigned char *addr, char addr_len, unsigned char *rx_buff, int length, rt_event_t *event){
    int irq = hal_irq_disable();
    _i2c_seq[seq_index++] = I2C_CMD_START;
    _i2c_seq[seq_index++] = I2C_CMD_WR;
    _i2c_seq[seq_index++] = dev_i2c->i2c_conf.addr_cs;
    for (int i=0; i<addr_len; i++){
        _i2c_seq[seq_index++] = I2C_CMD_WR;
        _i2c_seq[seq_index++] = addr[i];
    }
    _i2c_seq[seq_index++] = I2C_CMD_START;
    _i2c_seq[seq_index++] = I2C_CMD_WR;
    _i2c_seq[seq_index++] = (dev_i2c->i2c_conf.addr_cs|0x1);
    _i2c_seq[seq_index++] = I2C_CMD_RPT;
    _i2c_seq[seq_index++] = length-1;
    _i2c_seq[seq_index++] = I2C_CMD_RD_ACK;
    _i2c_seq[seq_index++] = I2C_CMD_RD_NACK;
    _i2c_seq[seq_index++] = I2C_CMD_STOP;
    _i2c_seq[seq_index++] = I2C_CMD_WAIT;
    _i2c_seq[seq_index++] = 0x1;
    rt_event_t *call_event = __rt_wait_event_prepare(event);
    rt_periph_copy_init(&call_event->copy, 0);
    rt_periph_copy_init(&dev_i2c->i2c_conf.writeCopy, 0);
    rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_i2c->channel) + 0 , (unsigned int ) rx_buff, length,  UDMA_CHANNEL_CFG_EN, call_event);
    rt_periph_copy(&dev_i2c->i2c_conf.writeCopy, UDMA_CHANNEL_ID(dev_i2c->channel) + 1 , (unsigned int) &_i2c_seq, seq_index, UDMA_CHANNEL_CFG_EN, 0);
    __rt_wait_event_check(event, call_event);
    seq_index = 0;
    hal_irq_restore(irq);
}

void rt_i2c_write(rt_i2c_t *dev_i2c, unsigned int addr, unsigned char value, rt_event_t *event){
  int irq = hal_irq_disable();

  _writeReg.addr_cs_w = dev_i2c->i2c_conf.addr_cs;
  _writeReg.addr_MSB = ((addr >> 8) & 0xFF);
  _writeReg.addr_LSB = (addr & 0xFF);
  _writeReg.data = value;
  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_i2c->channel) + 1 , (unsigned int) &_writeReg, sizeof(i2c_write_t), UDMA_CHANNEL_CFG_EN, call_event);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

void rt_i2c_read(rt_i2c_t *dev_i2c, unsigned int addr, unsigned char *rx_buff, rt_event_t *event){
  int irq = hal_irq_disable();

  _readReg.addr_cs_w = dev_i2c->i2c_conf.addr_cs;
  _readReg.addr_cs_r = (dev_i2c->i2c_conf.addr_cs|0x01);
  _readReg.addr_MSB = ((addr >> 8) & 0xFF);
  _readReg.addr_LSB = (addr & 0xFF);
  _readReg.repeat = 0;
  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy_init(&dev_i2c->i2c_conf.writeCopy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_i2c->channel) + 0 , (unsigned int ) rx_buff, 1,  UDMA_CHANNEL_CFG_EN, call_event);
  // Like here is api for read, so we will have this event ONLY when the read is finished
  rt_periph_copy(&dev_i2c->i2c_conf.writeCopy, UDMA_CHANNEL_ID(dev_i2c->channel) + 1 , (unsigned int)&_readReg, sizeof(i2c_read_t),  UDMA_CHANNEL_CFG_EN, 0);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

void i2c_read_burst(rt_i2c_t *dev_i2c, unsigned int addr, unsigned char *rx_buff, unsigned char length, rt_event_t *event){
  int irq = hal_irq_disable();

  _readReg.addr_cs_w = dev_i2c->i2c_conf.addr_cs;
  _readReg.addr_cs_r = (dev_i2c->i2c_conf.addr_cs|0x01);
  _readReg.addr_MSB = ((addr >> 8) & 0xFF);
  _readReg.addr_LSB = (addr & 0xFF);
  _readReg.repeat = (length-1);
  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy_init(&dev_i2c->i2c_conf.writeCopy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_i2c->channel) + 0 , (unsigned int) rx_buff, length, UDMA_CHANNEL_CFG_EN, call_event);
  // Like here is api for read, so we will have this event ONLY when the read is finished
  rt_periph_copy(&dev_i2c->i2c_conf.writeCopy, UDMA_CHANNEL_ID(dev_i2c->channel) + 1  , (unsigned int)&_readReg, sizeof(i2c_read_t), UDMA_CHANNEL_CFG_EN, call_event);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

void rt_i2c_conf(rt_i2c_t *dev_i2c, unsigned char addr_cs, unsigned int clk_divider, rt_event_t *event){
  int irq = hal_irq_disable();
  unsigned char i2c_id;
  if (dev_i2c->channel == (RT_PERIPH_I2C0_DATA>>1)) i2c_id = 0;
  else i2c_id = 1;

  plp_udma_cg_set(plp_udma_cg_get() | (1<<ARCHI_UDMA_I2C_ID(i2c_id)));
  if (i2c_id){
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(1)*2);
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(1)*2+1);
  }else{
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(0)*2);
    soc_eu_fcEventMask_setEvent(ARCHI_UDMA_I2C_ID(0)*2+1);
  }
  dev_i2c->i2c_conf.i2c_id = i2c_id;
  dev_i2c->i2c_conf.addr_cs = addr_cs;
  dev_i2c->i2c_conf.clk_divider = clk_divider;
  _i2cConf.value_MSB = (clk_divider >> 8) & 0xFF;
  _i2cConf.value_LSB = clk_divider & 0xFF;

  rt_event_t *call_event = __rt_wait_event_prepare(event);
  rt_periph_copy_init(&call_event->copy, 0);
  rt_periph_copy(&call_event->copy, UDMA_CHANNEL_ID(dev_i2c->channel) + 1 , (unsigned int) &_i2cConf, sizeof(i2c_config_t), UDMA_CHANNEL_CFG_EN, call_event);
  __rt_wait_event_check(event, call_event);
  hal_irq_restore(irq);
}

rt_i2c_t *__rt_i2c_open_channel(int channel, rt_i2c_conf_t *i2c_conf, rt_event_t *event){
  rt_i2c_t *i2c = NULL;
  i2c = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_i2c_t));
  if (i2c == NULL) goto error;

  i2c->dev = NULL;
  i2c->channel = channel;
  memcpy(&i2c->i2c_conf, i2c_conf, sizeof(rt_i2c_conf_t));
  rt_periph_copy_init(&i2c->i2c_conf.writeCopy, 0);

  return i2c;

error:
  rt_free(RT_ALLOC_FC_DATA, (void *) i2c, sizeof(rt_i2c_t));
  return NULL;
}

rt_i2c_t *rt_i2c_open(char *dev_name, rt_i2c_conf_t *i2c_conf, rt_event_t *event){
  rt_i2c_t *i2c = NULL;
  rt_dev_t *dev = rt_dev_get(dev_name);
  if (dev == NULL) goto error;
  i2c = rt_alloc(RT_ALLOC_FC_DATA, sizeof(rt_i2c_t));
  if (i2c == NULL) goto error;

  i2c->dev = dev;
  i2c->channel = dev->channel;
  memcpy(&i2c->i2c_conf, i2c_conf, sizeof(rt_i2c_conf_t));
  rt_periph_copy_init(&i2c->i2c_conf.writeCopy, 0);
  return i2c;

error:
  rt_free(RT_ALLOC_FC_DATA, (void *) i2c, sizeof(rt_i2c_t));
  return NULL;
}

void rt_i2c_close (rt_i2c_t *dev_i2c, rt_event_t *event){
  if (dev_i2c != NULL) {
    rt_free(RT_ALLOC_FC_DATA, (void *)dev_i2c, sizeof(rt_i2c_t));
  }
}

