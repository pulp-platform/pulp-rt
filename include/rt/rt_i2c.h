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

#include "rt/rt_api.h"

/**
* @ingroup groupDrivers
*/


/**
 * @defgroup I2C I2C driver
 *
 * The I2C driver provides support for transferring data between an external I2C device
 * and the chip running this driver.
 *
 */

/**
 * @addtogroup I2C
 * @{
 */

/**@{*/


/** \brief Open an I2C device.
 *
 * This function must be called before the I2C device can be used. It will do all the needed configuration to make it
 * usable and also return a handle used to refer to this opened device when calling other functions.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param dev_name  The device name in case the connection must be configured automatically. This name should correspond to the one used to configure the devices managed by the runtime. If it is NULL, all connection information must be provided in the configuration structure (interface id and chip select).
 * \param conf      A pointer to the I2C configuration. Can be NULL to take default configuration.
 * \param event     The event used for managing termination.
 * \return          NULL if the device is not found, or a handle identifying the device and which can be used with other functions.
 */
rt_i2c_t *rt_i2c_open(char *dev_name, rt_i2c_conf_t *conf, rt_event_t *event);

/** \brief Close an opened I2C device.
 *
 * This function can be called to close an opened I2C device once it is not needed anymore, in order to free
 * all allocated resources. Once this function is called, the device is not accessible anymore and must be opened again before being used.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param handle    The handler of the device which was returned when the device was opened.
 * \param event     The event used for managing termination.
 */
void rt_i2c_close (rt_i2c_t *handle, rt_event_t *event);

/** \brief Configure the I2C interface.
 *
 * This function can be called to configure an opened I2C interface.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param handle        The handler of the device which was returned when the device was opened.
 * \param addr_cs       The 7 bits (MSB) i2c slave address. Please left the LSB 0.
 * \param clk_divider   The clock divider of the i2c interface. freq_interface = 50MHz/(clk_divider*2)
 * \param event         The event used for managing termination.
 */
void rt_i2c_conf(rt_i2c_t *handle, unsigned char addr_cs, unsigned int clk_divider, rt_event_t *event);

/** \brief Enqueue a write copy to the I2C (from Chip to I2C device).
 *
 * This function can be used to send 1 byte data to the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 *
 * \param handle        The handler of the device which was returned when the device was opened.
 * \param addr          The address (8 bits) specified in the I2C device for this write operation.
 * \param value         The value should be writen to the address
 * \param event         The event used for managing termination.
 */
void rt_i2c_write(rt_i2c_t *handle, unsigned int addr, unsigned char value, rt_event_t *event);

/** \brief Enqueue a read copy to the I2C (from Chip to I2C device).
 *
 * This function can be used to read 1 byte data from the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 *
 * \param handle        The handler of the device which was returned when the device was opened.
 * \param addr          The address (16 bits) specified in the I2C device for this read operation.
 * \param rx_data       The address in the chip where the received data must be written.
 * \param event         The event used for managing termination.
 */
void rt_i2c_read(rt_i2c_t *handle, unsigned int addr, unsigned char *rx_data, rt_event_t *event);

/** \brief Enqueue a burst read copy from the I2C (from I2C device to chip).
 *
 * This function can be used to read several bytes of data from the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 *
 * \param handle        The handler of the device which was returned when the device was opened.
 * \param addr          The address (16 bits) specified in the I2C device for this read operation.
 * \param rx_data       The address in the chip where the received data must be written.
 * \param length        The size in bytes of the copy
 * \param event         The event used for managing termination.
 */
void i2c_read_burst(rt_i2c_t *handle, unsigned int addr, unsigned char *rx_buff, unsigned char length, rt_event_t *event);

/** \brief Enqueue a burst read copy from the I2C (from I2C device to chip).
 *
 * This function can be used to read at least 1 byte of data from the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 *
 * \param handle        The handler of the device which was returned when the device was opened.
 * \param addr          A pointer for the address specified in the I2C deveice.
 * \param addr_len      The size in byte of this address above.
 * \param rx_data       The address in the chip where the received data must be written.
 * \param length        The size in bytes of the copy
 * \param event         The event used for managing termination.
 */
void rt_i2c_read_seq(rt_i2c_t *handle, unsigned char *addr, char addr_len, unsigned char *rx_data, int length, rt_event_t *event);

/** \brief Enqueue a burst write copy to the I2C (from chip to I2C device).
 *
 * This function can be used to write at least 1 byte of data to the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 *
 * \param handle        The handler of the device which was returned when the device was opened.
 * \param addr          A pointer for the address specified in the I2C deveice.
 * \param addr_len      The size in byte of this address above.
 * \param tx_data       The address in the chip where the data to be sent.
 * \param length        The size in bytes of the copy
 * \param event         The event used for managing termination.
 */
void rt_i2c_write_seq(rt_i2c_t *dev_i2c, unsigned char *addr, char addr_len, unsigned char *tx_data, int length, rt_event_t *event);


//!@}

/**
 * @}
 */

/// @cond IMPLEM
rt_i2c_t *__rt_i2c_open_channel(int channel, rt_i2c_conf_t *i2c_conf, rt_event_t *event);
/// @endcond

#endif


