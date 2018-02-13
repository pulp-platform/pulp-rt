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

static RT_L2_DATA rt_i2c_t __rt_i2c[ARCHI_UDMA_NB_I2C];
static RT_L2_DATA unsigned char _i2c_seq[20];
static RT_FC_DATA unsigned char seq_index = 0;

typedef struct __i2c_arg_s{
    rt_periph_copy_t *copy;
    rt_i2c_t *dev_i2c;
    unsigned char *data;
    unsigned int len;
}__i2c_arg_t;

static inline int __rt_i2c_id(int periph_id)
{
    return periph_id - ARCHI_UDMA_I2C_ID(0);
}

static int __rt_i2c_get_div(int i2c_freq)
{
    // Round-up the divider to obtain an SPI frequency which is below the maximum
    int div = (__rt_freq_periph_get() + i2c_freq - 1)/i2c_freq;
    // The SPIM always divide by 2 once we activate the divider, thus increase by 1
    // in case it is even to not go avove the max frequency.
    if (div & 1) div += 1;
    div >>= 1;
    return div;
}

static void __rt_i2c_write_data(__i2c_arg_t *i2c_arg)
{
    int irq = hal_irq_disable();

    rt_periph_copy(i2c_arg->copy, i2c_arg->dev_i2c->channel + 1 , (unsigned int) i2c_arg->data, i2c_arg->len, UDMA_CHANNEL_CFG_EN, NULL);

    _i2c_seq[seq_index++] = I2C_CMD_STOP;
    _i2c_seq[seq_index++] = I2C_CMD_WAIT;
    _i2c_seq[seq_index++] = 0x1;
    hal_irq_restore(irq);
}

void rt_i2c_write(rt_i2c_t *dev_i2c, unsigned char *addr, char addr_len, unsigned char *data, int length, rt_event_t *event){
    int irq = hal_irq_disable();
    __i2c_arg_t i2c_write_arg;

    _i2c_seq[seq_index++] = I2C_CMD_START;
    _i2c_seq[seq_index++] = I2C_CMD_WR;
    _i2c_seq[seq_index++] = dev_i2c->cs;
    for (int i=addr_len; i>0; i--){
        _i2c_seq[seq_index++] = I2C_CMD_WR;
        _i2c_seq[seq_index++] = addr[i-1];
    }
    if (length > 1){
        _i2c_seq[seq_index++] = I2C_CMD_RPT;
        _i2c_seq[seq_index++] = length;
    }
    _i2c_seq[seq_index++] = I2C_CMD_WR;

    rt_event_t *call_event = __rt_wait_event_prepare(event);
    rt_periph_copy_init(&call_event->copy, 0);
    i2c_write_arg.copy = &call_event->copy;
    i2c_write_arg.dev_i2c = dev_i2c;
    i2c_write_arg.data = data;
    i2c_write_arg.len = length;

    rt_periph_copy(&call_event->copy, dev_i2c->channel + 1 , (unsigned int) &_i2c_seq, seq_index, UDMA_CHANNEL_CFG_EN, rt_event_get(NULL, (void *)__rt_i2c_write_data, &i2c_write_arg));
    seq_index = 0;
    hal_irq_restore(irq);

    rt_event_execute(NULL, 1);

    irq = hal_irq_disable();
    rt_periph_copy(&call_event->copy, dev_i2c->channel + 1 , (unsigned int) &_i2c_seq, seq_index, UDMA_CHANNEL_CFG_EN, call_event);
    hal_irq_restore(irq);
    __rt_wait_event_check(event, call_event);
    seq_index = 0;
}

void rt_i2c_read(rt_i2c_t *dev_i2c, unsigned char *addr, char addr_len, unsigned char *rx_buff, int length, unsigned char stop, rt_event_t *event){
    int irq = hal_irq_disable();
    _i2c_seq[seq_index++] = I2C_CMD_START;
    _i2c_seq[seq_index++] = I2C_CMD_WR;
    _i2c_seq[seq_index++] = dev_i2c->cs;
    for (int i=addr_len; i>0; i--){
        _i2c_seq[seq_index++] = I2C_CMD_WR;
        _i2c_seq[seq_index++] = addr[i-1];
    }
    if (stop) _i2c_seq [seq_index++] = I2C_CMD_STOP;
    _i2c_seq[seq_index++] = I2C_CMD_START;
    _i2c_seq[seq_index++] = I2C_CMD_WR;
    _i2c_seq[seq_index++] = (dev_i2c->cs|0x1);
    _i2c_seq[seq_index++] = I2C_CMD_RPT;
    _i2c_seq[seq_index++] = length-1;
    _i2c_seq[seq_index++] = I2C_CMD_RD_ACK;
    _i2c_seq[seq_index++] = I2C_CMD_RD_NACK;
    _i2c_seq[seq_index++] = I2C_CMD_STOP;
    _i2c_seq[seq_index++] = I2C_CMD_WAIT;
    _i2c_seq[seq_index++] = 0x2;

    rt_event_t *call_event = __rt_wait_event_prepare(event);
    rt_periph_copy_init(&call_event->copy, 0);
    rt_periph_dual_copy(&call_event->copy, dev_i2c->channel, (unsigned int)_i2c_seq, seq_index, (int)rx_buff, length, UDMA_CHANNEL_CFG_EN, call_event);
    __rt_wait_event_check(event, call_event);

    seq_index = 0;
    hal_irq_restore(irq);
}

void rt_i2c_freq_set(rt_i2c_t *dev_i2c, unsigned int frequency, rt_event_t *event)
{
    int irq = hal_irq_disable();

    dev_i2c->div = __rt_i2c_get_div(frequency);
    _i2c_seq[seq_index++] = I2C_CMD_CFG;
    _i2c_seq[seq_index++] = (dev_i2c->div >> 8) & 0xFF;
    _i2c_seq[seq_index++] = (dev_i2c->div & 0xFF);

    rt_event_t *call_event = __rt_wait_event_prepare(event);
    rt_periph_copy_init(&call_event->copy, 0);
    rt_periph_copy(&call_event->copy, dev_i2c->channel + 1 , (unsigned int) &_i2c_seq, seq_index, UDMA_CHANNEL_CFG_EN, call_event);
    __rt_wait_event_check(event, call_event);
    seq_index = 0;
    hal_irq_restore(irq);
}

rt_i2c_t *rt_i2c_open(char *dev_name, rt_i2c_conf_t *i2c_conf, rt_event_t *event)
{
    int irq = hal_irq_disable();

    __rt_padframe_init();


    rt_i2c_conf_t def_conf;

    if (i2c_conf == NULL)
    {
        i2c_conf = &def_conf;
        rt_i2c_conf_init(i2c_conf);
    }

    int channel = -1;

    if (i2c_conf->id != -1)
    {
        rt_trace(RT_TRACE_DEV_CTRL, "[I2C] Opening i2c device (id: %d)\n", i2c_conf->id);
        channel = ARCHI_UDMA_I2C_ID(i2c_conf->id);
    }
    else if (dev_name != NULL)
    {
        rt_trace(RT_TRACE_DEV_CTRL, "[I2C] Opening i2c device (name: %s)\n", dev_name);

        rt_dev_t *dev = rt_dev_get(dev_name);
        if (dev == NULL) goto error;

        channel = dev->channel;
    }

    if (channel == -1) goto error;

    rt_i2c_t *i2c = &__rt_i2c[__rt_i2c_id(channel)];
    if (i2c->open_count > 0) goto error;

    i2c->open_count++;
    i2c->channel = channel*2;
    i2c->cs = i2c_conf->cs;
    i2c->max_baudrate = i2c_conf->max_baudrate;

    plp_udma_cg_set(plp_udma_cg_get() | (1<<channel));
    soc_eu_fcEventMask_setEvent(channel*2);
    soc_eu_fcEventMask_setEvent(channel*2 + 1);

    if (event == NULL){
        rt_event_t *call_event = rt_event_get_blocking(NULL);
        rt_i2c_freq_set(i2c, i2c->max_baudrate, call_event);
        rt_event_wait(call_event);
    }else{
        rt_i2c_freq_set(i2c, i2c->max_baudrate, event);
    }
    hal_irq_restore(irq);

    return i2c;

error:
    rt_warning("[I2C] Failed to open I2C device\n");
    return NULL;
}

void rt_i2c_close (rt_i2c_t *i2c, rt_event_t *event){
    if (i2c != NULL)
    {
        plp_udma_cg_set(plp_udma_cg_get() & ~(1<<i2c->channel));
        i2c->open_count = 0;
    }
}

void rt_i2c_conf_init(rt_i2c_conf_t *conf)
{
    conf->cs = -1;
    conf->id = -1;
    conf->max_baudrate = 200000;
}

RT_FC_BOOT_CODE void __attribute__((constructor)) __rt_i2c_init()
{
    for (int i=0; i<ARCHI_UDMA_NB_I2C; i++)
    {
        __rt_i2c[i].open_count = 0;
    }
}

