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

#ifndef __PMSIS_DRIVERS_I2C_H__
#define __PMSIS_DRIVERS_I2C_H__


/**
* @ingroup groupDrivers
*/


/**
 * @defgroup I2C I2C
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

/// @cond IMPLEM

#define __PI_I2C_CTRL_SET_MAX_BAUDRATE_BIT 0

/// @endcond

/** \struct pi_i2c_conf_t
 * \brief I2C master configuration structure.
 *
 * This structure is used to pass the desired I2C configuration to the runtime when opening a device.
 */
typedef struct pi_i2c_conf {
	unsigned char itf;            /*!< Specifies on which I2C interface the device is connected. */
    signed char cs;               /*!< i2c slave address (7 bits on MSB), the runtime will take care of the LSB of READ and Write. */
    unsigned int  max_baudrate;   /*!< Maximum baudrate for the I2C bitstream which can be used with the opened device . */
} pi_i2c_conf_t;


/** \enum pi_i2c_xfer_prop_e
 * \brief Properties for I2C transfers
 *
 * This is used to know how the endianness must be applied.
 */
typedef enum {
  PI_I2C_XFER_STOP = 0,    /*!< Generate a STOP bit at the end of the transfer. */
  PI_I2C_XFER_PENDING = 1  /*!< Don't generate a STOP bit at the end of the transfer. */
} pi_i2c_xfer_prop_e;

/** \enum pi_i2c_control_e
 * \brief Commands for pi_i2c_control.
 *
 * This is used to tell which command to execute through pi_i2c_control.
 */
typedef enum {
  PI_I2C_CTRL_SET_MAX_BAUDRATE  = 1 << __PI_I2C_CTRL_SET_MAX_BAUDRATE_BIT, /*!< Change maximum baudrate. */
} pi_i2c_control_e;



/** \brief Open an I2C device.
 *
 * This function must be called before the I2C device can be used. It will do all the needed configuration to make it
 * usable and also return a handle used to refer to this opened device when calling other functions.
 *
 * \param device  A pointer to the structure describing the device. This structure muts be allocated by the caller and kept alive until the device is closed.
 * \return          0 if it succeeded or -1 if it failed.
 */
int pi_i2c_open(struct pi_device *device);

/** \brief Initialize an I2C configuration with default values.
 *
 * This function can be called to get default values for all parameters before setting some of them.
 * The structure containing the configuration must be kept alive until the I2C device is opened.
 *
 * \param conf A pointer to the I2C configuration.
 */
void pi_i2c_conf_init(pi_i2c_conf_t *conf);

/** \brief Close an opened I2C device.
 *
 * This function can be called to close an opened I2C device once it is not needed anymore, in order to free all allocated resources. Once this function is called, the device is not accessible anymore and must be opened again before being used.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param device  A pointer to the structure describing the device.
 */
void pi_i2c_close (struct pi_device *device);

/** \brief Dynamically change the device configuration.
 *
 * This function can be called to change part of the device configuration after it has been opened.
 *
 * \param device  A pointer to the structure describing the device.
 * \param cmd       The command which specifies which parameters of the driver to modify and for some of them also their values.
 * \param arg       An additional value which is required for some parameters when they are set.
 */
void pi_i2c_control(struct pi_device *device, pi_i2c_control_e cmd, uint32_t arg);

/** \brief Enqueue a burst read copy from the I2C (from I2C device to chip).
 *
 * This function can be used to read at least 1 byte of data from the I2C device.
 * The copy will make a synchronous transfer between the I2C and one of the chip memory.
 *
 * \param device  A pointer to the structure describing the device.
 * \param rx_buff       The address in the chip where the received data must be written.
 * \param length        The size in bytes of the copy
 * \param xfer_pending  If 1, the stop bit is not generated so that the same transfer can be continued with another call to pi_i2c_read or pi_i2c_write 
 */
void pi_i2c_read(struct pi_device *device, uint8_t *rx_buff, int length, int xfer_pending);

/** \brief Enqueue an asynchronous burst read copy from the I2C (from I2C device to chip).
 *
 * This function can be used to read at least 1 byte of data from the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 *
 * \param device  A pointer to the structure describing the device.
 * \param rx_buff       The address in the chip where the received data must be written.
 * \param length        The size in bytes of the copy
 * \param xfer_pending  If 1, the stop bit is not generated so that the same transfer can be continued with another call to pi_i2c_read or pi_i2c_write.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_i2c_read_async(struct pi_device *device, uint8_t *rx_buff, int length, int xfer_pending, pi_task_t *task);

/** \brief Enqueue a burst write copy to the I2C (from chip to I2C device).
 *
 * This function can be used to write at least 1 byte of data to the I2C device.
 * The copy will make a synchronous transfer between the I2C and one of the chip memory.
 *
 * \param device    A pointer to the structure describing the device.
 * \param tx_data       The address in the chip where the data to be sent.
 * \param length        The size in bytes of the copy
 * \param xfer_pending  If 1, the stop bit is not generated so that the same transfer can be continued with another call to pi_i2c_read or pi_i2c_write 
 */
void pi_i2c_write(struct pi_device *device, uint8_t *tx_data, int length, int xfer_pending);

/** \brief Enqueue a burst write copy to the I2C (from chip to I2C device).
 *
 * This function can be used to write at least 1 byte of data to the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 *
 * \param device    A pointer to the structure describing the device.
 * \param tx_data       The address in the chip where the data to be sent.
 * \param length        The size in bytes of the copy
 * \param xfer_pending  If 1, the stop bit is not generated so that the same transfer can be continued with another call to pi_i2c_read or pi_i2c_write.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_i2c_write_async(struct pi_device *device, uint8_t *tx_data, int length, int xfer_pending, pi_task_t *task);

/** \brief Enqueue a burst write copy to the I2C without START bit and address (from chip to I2C device).
 *
 * This function can be used to write at least 1 byte of data to the I2C device.
 * The copy will make a synchronous transfer between the I2C and one of the chip memory.
 * Compared to pi_i2c_write, the START bit is not generated and the slave
 * address is not sent.
 *
 * \param device    A pointer to the structure describing the device.
 * \param tx_data       The address in the chip where the data to be sent.
 * \param length        The size in bytes of the copy
 * \param xfer_pending  If 1, the stop bit is not generated so that the same transfer can be continued with another call to pi_i2c_read or pi_i2c_write 
 */
void pi_i2c_write_append(struct pi_device *device, uint8_t *tx_data, int length, int xfer_pending);

/** \brief Enqueue a burst write copy to the I2C without START bit and address (from chip to I2C device).
 *
 * This function can be used to write at least 1 byte of data to the I2C device.
 * The copy will make an asynchronous transfer between the I2C and one of the chip memory.
 * Compared to pi_i2c_write, the START bit is not generated and the slave
 * address is not sent.
 *
 * \param device    A pointer to the structure describing the device.
 * \param tx_data       The address in the chip where the data to be sent.
 * \param length        The size in bytes of the copy
 * \param xfer_pending  If 1, the stop bit is not generated so that the same transfer can be continued with another call to pi_i2c_read or pi_i2c_write 
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_i2c_write_append_async(struct pi_device *device, uint8_t *tx_data, int length, int xfer_pending, pi_task_t *task);

//!@}

/**
 * @}
 */




#endif


