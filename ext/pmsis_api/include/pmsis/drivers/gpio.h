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

#ifndef __PI_DRIVERS_GPIO_H__
#define __PI_DRIVERS_GPIO_H__

#include "pmsis/pmsis_types.h"

typedef enum
{
    PI_GPIO_INPUT = 0<<1,               /*!< The GPIO is an output, the chip can transmit a value. */
    PI_GPIO_OUTPUT = 1<<1,              /*!< The GPIO is an input, the chip can receive a value. */
    PI_GPIO_PULL_DISABLE = 0<<0,        /*!< Disable pull.  */
    PI_GPIO_PULL_ENABLE  = 1<<0,        /*!< Enable pull.  */
    PI_GPIO_DRIVE_STRENGTH_LOW  = 0<<2, /*!< Low drive strength.  */
    PI_GPIO_DRIVE_STRENGTH_HIGH = 1<<2  /*!< High drive strength. */
} pi_gpio_flags_e;

typedef enum
{
    PI_GPIO_NOTIF_FALL = 0x0,    /*!< IRQ on falling edge. */
    PI_GPIO_NOTIF_RISE = 0x1,    /*!< IRQ on rising edge. */
    PI_GPIO_NOTIF_EDGE = 0x2,    /*!< IRQ on a rising edge or a falling edge. */
    PI_GPIO_NOTIF_NONE = 0x3
} pi_gpio_notif_e;


struct pi_gpio_conf
{
    pi_device_e device;
    int32_t port;
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

#endif  /* __PI_DRIVERS_GPIO_H__ */
