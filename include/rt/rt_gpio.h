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

#ifndef __RT__RT_GPIO_H__
#define __RT__RT_GPIO_H__

#include "rt/rt_api.h"



/**
* @ingroup groupDrivers
*/



/**
 * @defgroup GPIO GPIO
 *
 * The GPIO driver provides support for controlling GPIOs.
 *
 */

/**
 * @addtogroup GPIO
 * @{
 */

/**@{*/


/** \enum rt_gpio_dir_e
 * \brief Direction of a GPIO.
 *
 * This is used to tell if the GPIO can receive or transmit a value.
 */
typedef enum {
  RT_GPIO_IS_OUT = 1,   /*!< The GPIO is an output, the chip can transmit a value. */
  RT_GPIO_IS_IN  = 0    /*!< The GPIO is an input, the chip can receive a value. */
} rt_gpio_dir_e;



/** \brief Initialize a GPIO.
 *
 * This function will do all the required initializations before the GPIO can
 * can be used.
 * This will for example make sure the corresponding pad is configured for this
 * GPIO.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \param gpio   GPIO number. This is an integer between 0 and 31 identifying the GPIO to init.
 */
void rt_gpio_init(uint8_t group, int gpio);



/** \brief Deinitialize a GPIO.
 *
 * This function will undo all initializations which were done when rt_gpio_init
 * was called for this GPIO.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \param gpio   GPIO number. This is an integer between 0 and 31 identifying the GPIO to deinit.
 */
void rt_gpio_deinit(uint8_t group, int gpio);



/** \brief Set direction for a group of GPIO.
 *
 * This function can be used to specify if the GPIO is an output or an input.
 * An input GPIO will allow sending data from the chip to another device while
 * an input will allow receiving data.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \param mask   A mask of GPIOs for which to set the direction.
 *               There is one bit per GPIO, bit 0 is GPIO 0 and bit 31 GPIO 31.
 *               The direction will be configured for all GPIOs which have their
 *               corresponding bit set to 1.
 * \param is_out A flag for choosing the direction of the gpio
 */
static inline void rt_gpio_set_dir(uint8_t group, uint32_t mask, rt_gpio_dir_e is_out);



/** \brief Set value for a group of GPIO.
 *
 * This function can be used to change the value of a group of GPIO outputs.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \param mask   A mask of GPIOs for which to set the value.
 *               There is one bit per GPIO, bit 0 is GPIO 0 and bit 31 GPIO 31.
 *               The value will be set for all GPIOs which have their
 *               corresponding bit set to 1.
 * \param value  The value to be set. This can be be either 0 or 1 as the same value
 *               is written to all selected GPIOs.
 */
static inline void rt_gpio_set_value(uint8_t group, uint32_t mask, uint8_t value);



/** \brief Get value of a group of GPIO.
 *
 * This function can be used to get the value of a group of GPIO inputs.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \return       The values for the whole group.
 *               There is one bit per GPIO, bit 0 is GPIO 0 and bit 31 GPIO 31.
 */
static inline uint32_t rt_gpio_get_value(uint8_t group);



/** \brief Get value of a single GPIO.
 *
 * This function can be used to get the value of a single GPIO.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \param gpio   The GPIO number. Must be between 0 and 31.
 * \return       The value of the GPIO, can be either 0 or 1.
 */
static inline uint8_t rt_gpio_get_pin_value(uint8_t group, uint8_t gpio);



/** \brief Set value of a single GPIO.
 *
 * This function can be used to change the value of a single GPIO.
 *
 * \param group  GPIO group. Must always be 0 for now.
 * \param gpio   The GPIO number. Must be between 0 and 31.
 * \param value  The value to be set. This can be be either 0 or 1.
 */
static inline void rt_gpio_set_pin_value(uint8_t group, uint8_t gpio, uint8_t value);



//!@}

/**
 * @} end of GPIO master
 */




/// @cond IMPLEM




static inline void rt_gpio_set_dir(uint8_t group, uint32_t mask, rt_gpio_dir_e is_out)
{
  hal_gpio_set_dir(mask, is_out);
}


static inline void rt_gpio_set_value(uint8_t group, uint32_t mask, uint8_t value)
{
  hal_gpio_set_value(mask, value);
}



static inline uint32_t rt_gpio_get_value(uint8_t group)
{
  return hal_gpio_get_value();
}



static inline uint8_t rt_gpio_get_pin_value(uint8_t group, uint8_t pin)
{
  return (rt_gpio_get_value(group) >> pin) & 1;
}

static inline void rt_gpio_set_pin_value(uint8_t group, uint8_t pin, uint8_t value)
{
  hal_gpio_set_pin_value(pin, value);
}

/// @endcond


#endif
