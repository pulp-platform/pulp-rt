/*
 * Copyright (c) 2019, GreenWaves Technologies, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of GreenWaves Technologies, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SDRVL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __PMSIS_PIN_CONFIG_H__
#define __PMSIS_PIN_CONFIG_H__

#include "pmsis.h"

/*!
 * @addtogroup Port
 * @{
 */

/*******************************************************************************
 * Variables, macros, structures,... definitions
 ******************************************************************************/

/*! @brief Internal resistor pull feature selection. */
enum _port_pull
{
    PORT_PULL_UP_DISABLE = 0U, /*!< Internal pull-up resistor disable. */
    PORT_PULL_UP_ENABLE  = 1U   /*!< Internal pull-up resistor enable. */
};

/*! @brief Drive strength configuration. */
enum _port_drive_strength
{
    PORT_LOW_DRIVE_STRENGTH  = 0U,  /*!< Low-drive strength configured.  */
    PORT_HIGH_DRIVE_STRENGTH = 1U,  /*!< High-drive strength configured. */
};

/*! @brief Pin mux selection */
typedef enum port_mux
{
    PORT_MUX_ALT0 = 0U,           /*!< Default */
    PORT_MUX_ALT1 = 1U,           /*!< Corresponding pin is configured as GPIO. */
    PORT_MUX_GPIO = PORT_MUX_ALT1,/*!< Corresponding pin is configured as GPIO. */
    PORT_MUX_ALT2 = 2U,           /*!< Chip-specific */
    PORT_MUX_ALT3 = 3U,           /*!< Chip-specific */
} port_mux_t;

/*! @brief Port pin configuration structure. */
typedef struct port_pin_config
{
    uint16_t pull_select;    /*!< No-pull/pull-down/pull-up select. */
    uint16_t drive_strength; /*!< Fast/slow drive strength configuration. */
    port_mux_t mux;          /*!< Pin mux selection. */
} port_pin_config_t;


/*******************************************************************************
 * API
******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif


/*@}*/

/*!
 * @brief Pin mux configuration.
 *
 * @param base   Port base pointer.
 * @param pin    Port pin number.
 * @param mux    Pin mux slot selection.
 *        - #uPORT_MuxAlt0            : Default.
 *        - #uPORT_MuxAlt1            : Set as GPIO.
 *        - #uPORT_MuxAlt2            : chip-specific.
 *        - #uPORT_MuxAlt3            : chip-specific.
 *
 * @note This function is recommended to reset the pin mux.
 *
 */
static inline void port_set_pin_mux(PORT_Type *base, uint32_t pin, port_mux_t mux)
{
    int reg_num = (pin >> 4) & 0x3;
    /* Positon in the target register */
    int pos = pin & 0xF;
    int val = base->PADFUN[reg_num];
    val &= ~(PORT_PADFUN_MUX_MASK << (pos << 1));
    base->PADFUN[reg_num] = val | (uint32_t)(mux << (pos << 1));
}


/*!
 * @brief Pin configuration.
 *
 * This function configures a pin, it sets PADCFG and PADFUN registers with values contained in port_pin_config_t.
 *
 * This is an example to define an input pin or output pin PADCFG and PADFUN configuration.
 * @code
 * // Define a digital input pin PADCFG and PADFUN configuration
 * port_pin_config_t config = {
 *      uPORT_PullUpEnable,
 *      uPORT_LowDriveStrength,
 *      uPORT_MuxAsGpio,
 * };
 * @endcode
 *
 * @param base   Port base pointer.
 * @param pin    Port pin number.
 * @param config Pointer to the structure port_pin_config_t.
 */
static inline void port_set_pin_config(PORT_Type *base, uint32_t pin, const port_pin_config_t *config)
{
    /* Set pin pull-up and drive strength*/
    int reg_num = pin >> 2;
    int pos = pin & 0x3;
    int val = base->PADCFG[reg_num];
    val &= ~((PORT_PADCFG_DRIVE_STRENGTH_MASK | PORT_PADCFG_PULL_EN_MASK) << (pos << 3));
    base->PADCFG[reg_num] = val | (uint32_t)((config->pull_select | config->drive_strength) << (pos << 3));

    port_set_pin_mux(base, pin, config->mux);
}

/*!
 * @brief Multiple pin configuration.
 *
 * This function configures multiple pins, it sets PADCFG and PADFUN registers with values contained in port_pin_config_t.
 * All pins are configured with same values. This function calls the function defined above.
 *
 * This is an example to define input pins or output pins PADCFG and PADFUN configuration.
 * @code
 * // Define a digital input pin PADCFG and PADFUN configuration
 * port_pin_config_t config = {
 *      uPORT_PullUp ,
 *      uPORT_LowDriveStrength,
 *      uPORT_MuxAsGpio,
 * };
 * @endcode
 *
 * @param base   Port base pointer.
 * @param mask   Mask of the pins to configure.
 * @param config Pointer to the structure port_pin_config_t.
 */
static inline void port_set_multiple_pins_config(PORT_Type *base, uint32_t mask,
        const port_pin_config_t *config)
{
    for (int pin = 0; pin < 32; pin++) {
        if(mask & (1 << pin))
            port_set_pin_config(base, pin, config);
    }
}

/*!
 * @brief Configures group pin muxing.
 *
 * @param base   Port base pointer.
 * @param mux    Pin muxing slot selection array for all pins.
 *
 * @note This function is recommended to reset the group pin mux.
 *
 */
static inline void port_set_group_pin_mux(PORT_Type *base, uint32_t mux[])
{
    base->PADFUN[0] = mux[0];
    base->PADFUN[1] = mux[1];
    base->PADFUN[2] = mux[2];
    base->PADFUN[3] = mux[3];
}

void pin_function(pin_name_e pin, int function);

void pin_mode(pin_name_e pin, pin_mode_e mode);

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif
