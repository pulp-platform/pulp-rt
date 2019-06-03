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

#ifndef __DRIVERS__SPI_H__
#define __DRIVERS__SPI_H__

#include <stdint.h>


/// @cond IMPLEM

#define __PI_SPI_CTRL_CPOL_BIT 0
#define __PI_SPI_CTRL_CPHA_BIT 2
#define __PI_SPI_CTRL_WORDSIZE_BIT 4
#define __PI_SPI_CTRL_ENDIANNESS_BIT 6
#define __PI_SPI_CTRL_SET_MAX_BAUDRATE_BIT 8

/// @endcond



/**
* @ingroup groupDrivers
*/



/**
 * @defgroup SPIM SPI
 *
 * The SPIM driver provides support for transferring data between an external SPIM device
 * and the chip running this driver.
 *
 */

/**
 * @addtogroup SPIM
 * @{
 */

/**@{*/




/** \enum pi_spi_wordsize_e
 * \brief Wordsize of the SPI bitstream elements.
 *
 * This is used to know how the endianness must be applied.
 */
typedef enum {
  PI_SPI_WORDSIZE_8 = 0,     /*!< Each element is 8 bits. Thus the endianness has no effect. */
  PI_SPI_WORDSIZE_32 = 1     /*!< Each element is 32 bits. The way each element is stored in memory can then be specified with the endianness. */
} pi_spi_wordsize_e;



/** \struct pi_spi_conf_t
 * \brief SPI master configuration structure.
 *
 * This structure is used to pass the desired SPI master configuration to the runtime when opening a device.
 */
struct pi_spi_conf {
  int max_baudrate;       /*!< Maximum baudrate for the SPI bitstream which can be used with the opened device . */
  char wordsize;          /*!< Wordsize of the elements in the bitstream. Can be PI_SPI_WORDSIZE_8 for 8 bits data or PI_SPI_WORDSIZE_32 for 32 bits data. This is used to interpret the endianness. */
  char big_endian;        /*!< If 1, the elements are stored in memory in a big-endian way, i.e. the most significant byte is stored at the lowest address. This is taken into account only if the wordsize is 32 bits. */
  char polarity;          /*!< Polarity of the clock. */
  char phase;             /*!< Phase of the clock. */
  signed char cs_gpio;    /*!< If it is different from -1, the specified number is used to drive a GPIO which is used as a chip select for the SPI device. The cs field is then ignored. */
  signed char cs;         /*!< If cs_gpio is -1, the normal chip select pins are used and this field specifies which one to use for the device. */
  signed char itf;         /*!< Specifies on which SPI interface the device is connected. */
};



/** \enum pi_spi_control_e
 * \brief Possible parameters which can be set through the pi_spi_control API function.
 *
 * This is used to reconfigure dynamically some of the parameters of an opened device.
 */
typedef enum {
  PI_SPI_CTRL_CPOL0             = 1 << __PI_SPI_CTRL_CPOL_BIT,             /*!< Set the clock polarity to 0. */
  PI_SPI_CTRL_CPOL1             = 2 << __PI_SPI_CTRL_CPOL_BIT,             /*!< Set the clock polarity to 1. */
  PI_SPI_CTRL_CPHA0             = 1 << __PI_SPI_CTRL_CPHA_BIT,             /*!< Set the clock phase to 0. */
  PI_SPI_CTRL_CPHA1             = 2 << __PI_SPI_CTRL_CPHA_BIT,             /*!< Set the clock phase to 1. */
  PI_SPI_CTRL_WORDSIZE_8        = 1 << __PI_SPI_CTRL_WORDSIZE_BIT,         /*!< Set the wordsize to 8 bits. */
  PI_SPI_CTRL_WORDSIZE_32       = 2 << __PI_SPI_CTRL_WORDSIZE_BIT,         /*!< Set the wordsize to 32 bits. */
  PI_SPI_CTRL_BIG_ENDIAN        = 1 << __PI_SPI_CTRL_ENDIANNESS_BIT,       /*!< Handle the elements in memory in a big-endian way. */
  PI_SPI_CTRL_LITTLE_ENDIAN     = 2 << __PI_SPI_CTRL_ENDIANNESS_BIT,       /*!< Handle the elements in memory in a little-endian way. */
  PI_SPI_CTRL_SET_MAX_BAUDRATE  = 1 << __PI_SPI_CTRL_SET_MAX_BAUDRATE_BIT, /*!< Change maximum baudrate. */
} pi_spi_control_e;



/** \enum pi_spi_cs_e
 * \brief Specifies chip select mode.
 *
 * 
 */
typedef enum {
  PI_SPI_CS_AUTO     = 0,    /*!< Handles the chip select automatically. It is set low just before the transfer is started and set back high when the transfer is finished. */
  PI_SPI_CS_KEEP     = 1,    /*!< Handle the chip select manually. It is set low just before the transfer is started and is kept low until the next transfer. */
  PI_SPI_CS_NONE     = 2,    /*!< Don't do anything with the chip select. */
} pi_spi_cs_e;



/** \brief Initialize an SPI master configuration with default values.
 *
 * This function can be called to get default values for all parameters before setting some of them.
 * The structure containing the configuration must be kept alive until the SPI device is opened.
 *
 * \param conf A pointer to the SPI master configuration.
 */
void pi_spi_conf_init(struct pi_spi_conf *conf);



/** \brief Open an SPI device.
 *
 * This function must be called before the SPI device can be used. It will do all the needed configuration to make it
 * usable and also return a handle used to refer to this opened device when calling other functions.
 *
 * \param device  A pointer to the structure describing the device. This structure muts be allocated by the caller and kept alive until the device is closed.
 * \return          0 if it succeeded or -1 if it failed.
 */
int pi_spi_open(struct pi_device *device);



/** \brief Close an opened SPI device.
 *
 * This function can be called to close an opened SPI device once it is not needed anymore, in order to free
 * all allocated resources. Once this function is called, the device is not accessible anymore and must be opened
 * again before being used.
 *
 * \param device  A pointer to the structure describing the device.
 */
void pi_spi_close(struct pi_device *device);



/** \brief Dynamically change the device configuration.
 *
 * This function can be called to change part of the device configuration after it has been opened.
 *
 * \param device  A pointer to the structure describing the device.
 * \param cmd       The command which specifies which parameters of the driver to modify and for some of them also their values.
 * \param arg       An additional value which is required for some parameters when they are set.
 */
void pi_spi_control(struct pi_device *device, pi_spi_control_e cmd, uint32_t arg);



/** \brief Enqueue a write copy to the SPI (from Chip to SPI device).
 *
 * This function can be used to send data to the SPI device.
 * The copy will make a synchronous transfer between the SPI and one of the chip memory.
 * This is using classic SPI transfer with MOSI and MISO lines. 
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data     The address in the chip where the data to be sent must be read.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 */
static inline void pi_spi_send(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode);

/** \brief Enqueue a write copy to the SPI (from Chip to SPI device).
 *
 * This function can be used to send data to the SPI device.
 * The copy will make an asynchronous transfer between the SPI and one of the chip memory.
 * This is using classic SPI transfer with MOSI and MISO lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data     The address in the chip where the data to be sent must be read.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
static inline void pi_spi_send_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode, pi_task_t *task);



/** \brief Enqueue a read copy to the SPI (from Chip to SPI device).
 *
 * This function can be used to receive data from the SPI device.
 * The copy will make a synchronous transfer between the SPI and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 * This is using classic SPI transfer with MOSI and MISO lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data        The address in the chip where the received data must be written.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 */
static inline void pi_spi_receive(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode);

/** \brief Enqueue a read copy to the SPI (from Chip to SPI device).
 *
 * This function can be used to receive data from the SPI device.
 * The copy will make an asynchronous transfer between the SPI and one of the chip memory.
 * An event can be specified in order to be notified when the transfer is finished.
 * This is using classic SPI transfer with MOSI and MISO lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data        The address in the chip where the received data must be written.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
static inline void pi_spi_receive_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode, pi_task_t *task);

/** \brief Enqueue a read and write copy to the SPI (using full duplex mode).
 *
 * This function can be used to send and receive data with the SPI device using full duplex mode.
 * The copy will make a synchronous transfer between the SPI and one of the chip memory.
 * This is using classic SPI transfer with MOSI and MISO lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param tx_data     The address in the chip where the data to be sent must be read.
 * \param rx_data     The address in the chip where the received data must be written.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 */
void pi_spi_transfer(struct pi_device *device, void *tx_data, void *rx_data, size_t len, pi_spi_cs_e cs_mode);

/** \brief Enqueue a read and write copy to the SPI (using full duplex mode).
 *
 * This function can be used to send and receive data with the SPI device using full duplex mode.
 * The copy will make an asynchronous transfer between the SPI and one of the chip memory.
 * This is using classic SPI transfer with MOSI and MISO lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param tx_data     The address in the chip where the data to be sent must be read.
 * \param rx_data     The address in the chip where the received data must be written.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_spi_transfer_async(struct pi_device *device, void *tx_data, void *rx_data, size_t len, pi_spi_cs_e cs_mode, pi_task_t *task);

/** \brief Enqueue a write copy to the SPI using quad spi (from Chip to SPI device).
 *
 * This function can be used to send data to the SPI device using quad SPI.
 * The copy will make a synchronous transfer between the SPI and one of the chip memory.
 * This is using quad SPI transfer with 4 data lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data        The address in the chip where the data to be sent must be read.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 */
static inline void pi_spi_send_qspi(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode);

/** \brief Enqueue a write copy to the SPI using quad spi (from Chip to SPI device).
 *
 * This function can be used to send data to the SPI device using quad SPI.
 * The copy will make an asynchronous transfer between the SPI and one of the chip memory.
 * This is using quad SPI transfer with 4 data lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data        The address in the chip where the data to be sent must be read.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
static inline void pi_spi_send_qspi_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode, pi_task_t *task);

/** \brief Enqueue a read copy to the SPI using quad SPI (from Chip to SPI device).
 *
 * This function can be used to receive data from the SPI device using quad SPI.
 * The copy will make a synchronous transfer between the SPI and one of the chip memory.
 * This is using quad SPI transfer with 4 data lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data        The address in the chip where the received data must be written.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 */
static inline void pi_spi_receive_qspi(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode);

/** \brief Enqueue a read copy to the SPI using quad SPI (from Chip to SPI device).
 *
 * This function can be used to receive data from the SPI device using quad SPI.
 * The copy will make an asynchronous transfer between the SPI and one of the chip memory.
 * This is using quad SPI transfer with 4 data lines.
 * Due to hardware constraints, the address of the buffer must be aligned on 4 bytes and the size must be a multiple of 4.
 *
 * \param device  A pointer to the structure describing the device.
 * \param data        The address in the chip where the received data must be written.
 * \param len         The size in bits of the copy.
 * \param cs_mode     The mode for managing the chip select.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
static inline void pi_spi_receive_qspi_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e cs_mode, pi_task_t *task);



//!@}

/**
 * @} end of SPI master
 */

/// @cond IMPLEM

void __pi_spi_send(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e mode);

void __pi_spi_send_async(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e mode, pi_task_t *task);

void __pi_spi_receive(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e mode);

void __pi_spi_receive_async(struct pi_device *device, void *data, size_t len, int qspi, pi_spi_cs_e mode, pi_task_t *task);

static inline void pi_spi_send(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode)
{
  __pi_spi_send(device, data, len, 0, mode);
}

static inline void pi_spi_send_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode, pi_task_t *task)
{
  __pi_spi_send_async(device, data, len, 0, mode, task);
}

static inline void pi_spi_receive(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode)
{
  __pi_spi_receive(device, data, len, 0, mode);
}

static inline void pi_spi_receive_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode, pi_task_t *task)
{
  __pi_spi_receive_async(device, data, len, 0, mode, task);
}

static inline void pi_spi_send_qspi(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode)
{
  __pi_spi_send(device, data, len, 1, mode);
}

static inline void pi_spi_send_qspi_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode, pi_task_t *task)
{
  __pi_spi_send_async(device, data, len, 1, mode, task);
}

static inline void pi_spi_receive_qspi(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode)
{
  __pi_spi_receive(device, data, len, 1, mode);
}

static inline void pi_spi_receive_qspi_async(struct pi_device *device, void *data, size_t len, pi_spi_cs_e mode, pi_task_t *task)
{
  __pi_spi_receive_async(device, data, len, 1, mode, task);
}


/// @endcond

#endif
