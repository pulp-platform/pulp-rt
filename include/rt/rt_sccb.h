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

#ifndef __SCCB_H__
#define __SCCB_H__

typedef struct {
    const unsigned char     config_cmd;
    unsigned char           value_MSB;
    unsigned char           value_LSB;
} _sccb_config_t;

typedef struct {
    const unsigned char     start;
    const unsigned char     cmd_w_ctrl;
    unsigned char           addr_cs_w;
    const unsigned char     cmd_w_addr;
    unsigned char           addr_LSB;
    const unsigned char     end_ad;
    const unsigned char     start_rd;
    const unsigned char     cmd_r_ctrl;
    unsigned char           addr_cs_r;
    const unsigned char     cmd_r_nack;
    const unsigned char     end;
    const unsigned char     cmd_wait;
    unsigned char           wait;
} _sccb_read_t;

typedef struct {
    const unsigned char     start_rd;
    const unsigned char     cmd_r_ctrl;
    unsigned char           addr_cs_r;
    const unsigned char     cmd_r_nack;
    const unsigned char     end;
    const unsigned char     cmd_wait;
    unsigned char           wait;
} _sccb_short_read_t;

typedef struct {
    const unsigned char     start;
    const unsigned char     cmd_w_ctrl;
    unsigned char           addr_cs_w;
    const unsigned char     cmd_w_addr;
    unsigned char           addr_LSB;
    const unsigned char     cmd_w_data;
    unsigned char           data;
    const unsigned char     end;
    const unsigned char     cmd_wait;
    unsigned char           wait;
} _sccb_write_t;


rt_i2c_t *rt_sccb_open(char *dev_name, rt_i2c_conf_t *sccb_conf, rt_event_t *event);
rt_i2c_t *__rt_sccb_open_channel(int channel, rt_i2c_conf_t *sccb_conf, rt_event_t *event);
void rt_sccb_close (rt_i2c_t *dev_sccb, rt_event_t *event);
void rt_sccb_conf(rt_i2c_t *dev_sccb, unsigned char addr_cs, unsigned int clkDivider, rt_event_t *event);
void rt_sccb_write(rt_i2c_t *dev_sccb, unsigned int addr, unsigned char value, rt_event_t *event);
void rt_sccb_read(rt_i2c_t *dev_sccb, unsigned int addr, unsigned char *rxBuff, rt_event_t *event);
void sccb_short_read(rt_i2c_t *dev_sccb, unsigned int addr, unsigned char *rxBuff, rt_event_t *event);

#endif

