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

#ifndef __DRIVERS__GPIO_H__
#define __DRIVERS__GPIO_H__

#include <stdint.h>

typedef enum {
  PI_GPIO_INPUT = 0<<1,   /*!< The GPIO is an output, the chip can transmit a value. */
  PI_GPIO_OUTPUT = 1<<1    /*!< The GPIO is an input, the chip can receive a value. */
} pi_gpio_flags_e;

typedef enum {
  PI_GPIO_NOTIF_EDGE = 3,    /*!< Notifications are sent when there is both a rising edge or a falling edge on the GPIO value. */
  PI_GPIO_NOTIF_RISE = 2,    /*!< Notifications are sent when there is a rising edge on the GPIO value. */
  PI_GPIO_NOTIF_FALL = 1,    /*!< Notifications are sent when there is a falling edge on the GPIO value. */
  PI_GPIO_NOTIF_NONE = 0	
} pi_gpio_notif_e;


struct pi_gpio_conf {
  int port;
};

void pi_gpio_conf_init(struct pi_gpio_conf *conf);

int pi_gpio_open(struct pi_device *device);

int pi_gpio_pin_configure(struct pi_device *device, int pin, pi_gpio_flags_e flags);

int pi_gpio_pin_write(struct pi_device *device, int pin, uint32_t value);

int pi_gpio_pin_read(struct pi_device *device, int pin, uint32_t *value);

int pi_gpio_pin_task_add(struct pi_device *device, int pin, pi_task_t *task, pi_gpio_notif_e flags);

int pi_gpio_pin_task_remove(struct pi_device *device, int pin);

void pi_gpio_pin_notif_configure(struct pi_device *device, int pin, pi_gpio_notif_e flags);

void pi_gpio_pin_notif_clear(struct pi_device *device, int pin);

int pi_gpio_pin_notif_get(struct pi_device *device, int pin);

int pi_gpio_mask_configure(struct pi_device *device, uint32_t mask, pi_gpio_flags_e flags);

int pi_gpio_mask_write(struct pi_device *device, uint32_t mask, uint32_t value);

int pi_gpio_mask_read(struct pi_device *device, uint32_t mask, uint32_t *value);

int pi_gpio_mask_task_add(struct pi_device *device, uint32_t mask, pi_task_t *task, pi_gpio_notif_e flags);

int pi_gpio_mask_task_remove(struct pi_device *device, uint32_t mask);

#endif