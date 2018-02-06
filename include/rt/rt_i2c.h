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

#ifndef __RT__RT_API_I2C_H__
#define __RT__RT_API_I2C_H__

typedef struct{
  const unsigned char     config_cmd;
  unsigned char           value_MSB;
  unsigned char           value_LSB;
} i2c_config_t;

typedef struct {
  const unsigned char     start;
  const unsigned char     cmd_w_ctrl;
  unsigned char           addr_cs_w;
  const unsigned char     cmd_w_addrI_MSB;
  unsigned char           addr_MSB;
  const unsigned char     cmd_w_addrI_LSB;
  unsigned char           addr_LSB;
  const unsigned char     start_rd;
  const unsigned char     cmd_r_ctrl;
  unsigned char           addr_cs_r;
  const unsigned char     cmd_rpt;
  unsigned char           repeat;
  const unsigned char     cmd_r_ack;
  const unsigned char     cmd_r_nack;
  const unsigned char     end;
  const unsigned char     cmd_wait;
  unsigned char           wait;
} i2c_read_t;

typedef struct {
  const unsigned char     start;
  const unsigned char     cmd_w_ctrl;
  unsigned char           addr_cs_w;
  const unsigned char     cmd_w_addrI_MSB;
  unsigned char           addr_MSB;
  const unsigned char     cmd_w_addrI_LSB;
  unsigned char           addr_LSB;
  const unsigned char     cmd_w_data;
  unsigned char           data;
  const unsigned char     end;
  const unsigned char     cmd_wait;
  unsigned char           wait;
} i2c_write_t;

typedef struct{
  unsigned char i2c_id;
  unsigned char addr_cs;
  unsigned int  clk_divider;
  rt_periph_copy_t writeCopy;
}rt_i2c_conf_t;

typedef struct rt_i2c_s {
  rt_dev_t *dev;
  int channel;
  rt_i2c_conf_t i2c_conf;
} rt_i2c_t;

rt_i2c_t *rt_i2c_open(char *dev_name, rt_i2c_conf_t *i2c_conf, rt_event_t *event);
rt_i2c_t *__rt_i2c_open_channel(int channel, rt_i2c_conf_t *i2c_conf, rt_event_t *event);
void rt_i2c_close (rt_i2c_t *dev_i2c, rt_event_t *event);
void rt_i2c_conf(rt_i2c_t *dev_i2c, unsigned char addr_cs, unsigned int clk_divider, rt_event_t *event);
void rt_i2c_write(rt_i2c_t *dev_i2c, unsigned int addr, unsigned char value, rt_event_t *event);
void rt_i2c_read(rt_i2c_t *dev_i2c, unsigned int addr, unsigned char *rx_buff, rt_event_t *event);
void i2c_read_burst(rt_i2c_t *dev_i2c, unsigned int addr, unsigned char *rx_buff, unsigned char length, rt_event_t *event);
void rt_i2c_read_seq(rt_i2c_t *dev_i2c, unsigned char *addr, char addr_len, unsigned char *rx_buff, int length, rt_event_t *event);
void rt_i2c_write_seq(rt_i2c_t *dev_i2c, unsigned char *data, int length, rt_event_t *event);
#endif


