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

#include "pmsis.h"

#define NB_GPIO_PORT ((ARCHI_NB_GPIO+31)/32)


typedef struct 
{
  int port;
} pi_gpio_t;

static pi_gpio_t __rt_gpio[NB_GPIO_PORT];



void pi_gpio_conf_init(struct pi_gpio_conf *conf)
{
  conf->port = 0;
}



int pi_gpio_open(struct pi_device *device)
{
  int irq = rt_irq_disable();

  struct pi_gpio_conf *conf = (struct pi_gpio_conf *)device->config;

  if (conf->port >= NB_GPIO_PORT)
    goto error;

  pi_gpio_t *gpio = &__rt_gpio[conf->port];

  device->data = (void *)gpio;

  gpio->port = conf->port;

  rt_irq_restore(irq);

  return 0;

error:
  rt_irq_restore(irq);
  return -1;
}


int pi_gpio_pin_configure(struct pi_device *device, int pin, pi_gpio_flags_e flags)
{
  return pi_gpio_mask_configure(device, 1<<pin, flags);
}

int pi_gpio_pin_write(struct pi_device *device, int pin, uint32_t value)
{
  int irq = rt_irq_disable();
  hal_gpio_set_pin_value(pin, value);
  rt_irq_restore(irq);
  return 0;
}

int pi_gpio_pin_read(struct pi_device *device, int pin, uint32_t *value)
{
  *value = (hal_gpio_get_value() >> pin) & 1;
  return 0;
}

int pi_gpio_pin_task_add(struct pi_device *device, int pin, pi_task_t *task, pi_gpio_task_flags_e flags)
{
  return 0;
}

int pi_gpio_pin_task_remove(struct pi_device *device, int pin)
{
  return 0;
}

int pi_gpio_mask_configure(struct pi_device *device, uint32_t mask, pi_gpio_flags_e flags)
{
  int irq = rt_irq_disable();
  int is_out = flags & PI_GPIO_OUTPUT;
  hal_gpio_set_dir(mask, is_out);

  if (is_out)
    hal_gpio_en_set(hal_gpio_en_get() & ~mask);
  else
    hal_gpio_en_set(hal_gpio_en_get() | mask);

  rt_irq_restore(irq);

  return 0;
}

int pi_gpio_mask_write(struct pi_device *device, uint32_t mask, uint32_t value)
{
  hal_gpio_set_value(mask, value);
  return 0;
}

int pi_gpio_mask_read(struct pi_device *device, uint32_t mask, uint32_t *value)
{
  *value = hal_gpio_get_value();
  return 0;
}

int pi_gpio_mask_task_add(struct pi_device *device, uint32_t mask, pi_task_t *task, pi_gpio_task_flags_e flags)
{
  return 0;
}

int pi_gpio_mask_task_remove(struct pi_device *device, uint32_t mask)
{
  return 0;
}
