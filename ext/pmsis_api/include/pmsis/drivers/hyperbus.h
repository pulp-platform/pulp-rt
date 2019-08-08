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

#ifndef __PI_DRIVERS_HYPERBUS_H__
#define __PI_DRIVERS_HYPERBUS_H__

#include "pmsis/pmsis_types.h"


/**
* @ingroup groupDrivers
*/



/**
 * @defgroup Hyperbus Hyperbus
 *
 * The Hyperbus driver provides support for transferring data between an external Hyperbus chip
 * and the processor running this driver.
 *
 * The driver provides a memory allocator for allocating and freeing memory in the Hyperbus chip
 * and an asynchronous API for transferring data.
 *
 */

/**
 * @addtogroup Hyperbus
 * @{
 */

/**@{*/

/** \enum pi_hyper_type_e
 * \brief Type of device connected to the hyperbus interface.
 *
 * This is used to know if the device is a flash or a RAM.
 */
typedef enum
{
    PI_HYPER_TYPE_FLASH,  /*!< Device is a flash. */
    PI_HYPER_TYPE_RAM   /*!< Device is a RAM. */
} pi_hyper_type_e;


/** \struct pi_hyper_conf
 * \brief Hyperbus configuration structure.
 *
 * This structure is used to pass the desired Hyperbus configuration to the runtime when opening the device.
 */
struct pi_hyper_conf
{
    pi_device_e device;  /* Device type. */
    uint32_t cs;         /*!< Chip select where the device is connected. */
    pi_hyper_type_e type;/*!< Type of device connected on the hyperbus interface. */
    signed char id;      /*!< If it is different from -1, this specifies on which hyperbus interface the device is connected. */
    signed int ram_size; /*!< Size of the ram. */
    uint32_t baudrate;
};

/** \brief Hyperbus request structure.
 *
 * This structure is used by the runtime to manage a cluster remote copy with the Hyperbus.
 * It must be instantiated once for each copy and must be kept alive until the copy is finished.
 * It can be instantiated as a normal variable, for example as a global variable, a local one on the stack,
 * or through a memory allocator.
 */
typedef struct pi_cl_hyper_req_s pi_cl_hyper_req_t;

/** \brief Hyperbus memory allocation request structure.
 *
 * This structure is used by the runtime to manage a cluster remote memory allocation
 * from the Hyperbus.
 * It must be instantiated once for each on-going allocation and must be kept alive
 * until the allocation is finished.
 * It can be instantiated as a normal variable, for example as a global variable, a local one on the stack,
 * or through a memory allocator.
 */
typedef struct pi_cl_hyperram_alloc_req_s pi_cl_hyperram_alloc_req_t;

/** \brief Hyperbus memory free request structure.
 *
 * This structure is used by the runtime to manage a cluster remote memory free
 * from the Hyperbus.
 * It must be instantiated once for each on-going free and must be kept alive
 * until the free is finished.
 * It can be instantiated as a normal variable, for example as a global variable, a local one on the stack,
 * or through a memory allocator.
 */
typedef struct pi_cl_hyperram_free_req_s pi_cl_hyperram_free_req_t;

/** \brief Initialize an Hyperbus configuration with default values.
 *
 * The structure containing the configuration must be kept alive until the camera is opened.
 *
 * \param conf A pointer to the Hyperbus configuration.
 */
void pi_hyper_conf_init(struct pi_hyper_conf *conf);

/** \brief Open an Hyperbus device.
 *
 * This function must be called before the Hyperbus device can be used. It will do all the needed configuration to make it
 * usable and also return a handle used to refer to this opened device when calling other functions.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param device    The device structure of the device to open.
 * \return          0 if the operation is successfull, -1 if there was an error
 */
int32_t pi_hyper_open(struct pi_device *device);

/** \brief Close an opened Hyperbus device.
 *
 * This function can be called to close an opened Hyperbus device once it is not needed anymore, in order to free
 * all allocated resources. Once this function is called, the device is not accessible anymore and must be opened
 * again before being used.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param device    The device structure of the device to close.
 */
void pi_hyper_close(struct pi_device *device);

/** \brief Enqueue a read copy to the Hyperbus (from Hyperbus to processor).
 *
 * The copy will make a transfer between the Hyperbus and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 */
void pi_hyper_read(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size);

/** \brief Enqueue an asynchronous read copy to the Hyperbus (from Hyperbus to processor).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_hyper_read_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, struct pi_task *task);

/** \brief Enqueue a write copy to the Hyperbus (from processor to Hyperbus).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 */
void pi_hyper_write(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size);

/** \brief Enqueue an asynchronous write copy to the Hyperbus (from processor to Hyperbus).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_hyper_write_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, struct pi_task *task);

/** \brief Enqueue a 2D read copy (rectangle area) to the Hyperbus (from Hyperbus to processor).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 */
void pi_hyper_read_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length);

/** \brief Enqueue an asynchronous 2D read copy (rectangle area) to the Hyperbus (from Hyperbus to processor).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_hyper_read_2d_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, struct pi_task *task);

/** \brief Enqueue a 2D write copy (rectangle area) to the Hyperbus (from processor to Hyperbus).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 */
void pi_hyper_write_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length);

/** \brief Enqueue an asynchronous 2D write copy (rectangle area) to the Hyperbus (from processor to Hyperbus).
 *
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_task for more details.
 */
void pi_hyper_write_2d_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, struct pi_task *task);




/** \brief Allocate Hyperram memory
 *
 * The allocated memory is 4-bytes aligned. The allocator uses some meta-data stored in the fabric controller memory
 * for every allocation so it is advisable to do as few allocations as possible to lower the memory overhead.
 *
 * \param device The device descriptor of the Hyperbus chip for which the memory must be allocated
 * \param size   The size in bytes of the memory to allocate
 * \return NULL if not enough memory was available, otherwise the address of the allocated chunk
 */
uint32_t pi_hyperram_alloc(struct pi_device *device, uint32_t size);

/** \brief Free Hyperram memory
 *
 * The allocator does not store any information about the allocated chunks, thus the size of the allocated
 * chunk to to be freed must be provided by the caller.
 *
 * \param device The device descriptor of the Hyperbus chip for which the memory must be freed
 * \param chunk  The allocated chunk to free
 * \param size   The size in bytes of the memory chunk which was allocated
 * \return 0 if the operation was successful, -1 otherwise
 */
int32_t pi_hyperram_free(struct pi_device *device, uint32_t chunk, uint32_t size);

/** \brief Enqueue a read copy to the Hyperbus from cluster side (from Hyperbus to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperBus read copy.
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param req         A pointer to the HyperBus request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyper_read(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, pi_cl_hyper_req_t *req);

/** \brief Enqueue a 2D read copy (rectangle area) to the Hyperbus from cluster side (from Hyperbus to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperBus read copy.
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param req         A pointer to the HyperBus request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyper_read_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, pi_cl_hyper_req_t *req);

/** \brief Wait until the specified hyperbus request has finished.
 *
 * This blocks the calling core until the specified cluster remote copy is finished.
 *
 * \param req       The request structure used for termination.
 */
static inline void pi_cl_hyper_read_wait(pi_cl_hyper_req_t *req);

/** \brief Enqueue a write copy to the Hyperbus from cluster side (from Hyperbus to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperBus write copy.
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param req         A pointer to the HyperBus request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyper_write(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, pi_cl_hyper_req_t *req);

/** \brief Enqueue a 2D write copy (rectangle area) to the Hyperbus from cluster side (from Hyperbus to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperBus write copy.
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param req         A pointer to the HyperBus request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyper_write_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, pi_cl_hyper_req_t *req);

/** \brief Wait until the specified hyperbus request has finished.
 *
 * This blocks the calling core until the specified cluster remote copy is finished.
 *
 * \param req       The request structure used for termination.
 */
static inline void pi_cl_hyper_write_wait(pi_cl_hyper_req_t *req);

/** \brief Enqueue a copy with the Hyperbus from cluster side.
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperBus copy.
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param ext2loc     1 if the copy is from HyperBus to the chip or 0 for the contrary.
 * \param req         A pointer to the HyperBus request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyper_copy(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, int ext2loc, pi_cl_hyper_req_t *req);

/** \brief Enqueue a 2D copy (rectangle area) with the Hyperbus from cluster side.
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperBus copy.
 * The copy will make an asynchronous transfer between the Hyperbus and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the Hyperbus chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param ext2loc     1 if the copy is from HyperBus to the chip or 0 for the contrary.
 * \param req         A pointer to the HyperBus request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyper_copy_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, int ext2loc, pi_cl_hyper_req_t *req);

/** \brief Allocate Hyperram memory from cluster
 *
 * The allocated memory is 4-bytes aligned. The allocator uses some meta-data stored in the fabric controller memory
 * for every allocation so it is advisable to do as few allocations as possible to lower the memory overhead.
 *
 * \param device The device descriptor of the Hyperbus chip for which the memory must be allocated
 * \param size   The size in bytes of the memory to allocate
 * \param req    The request structure used for termination.
 */
void pi_cl_hyperram_alloc(struct pi_device *device, uint32_t size, pi_cl_hyperram_alloc_req_t *req);

/** \brief Free Hyperram memory from cluster
 *
 * The allocator does not store any information about the allocated chunks, thus the size of the allocated
 * chunk to to be freed must be provided by the caller.
 * Can only be called from fabric-controller side.
 *
 * \param device The device descriptor of the Hyperbus chip for which the memory must be freed
 * \param chunk  The allocated chunk to free
 * \param size   The size in bytes of the memory chunk which was allocated
 * \param req    The request structure used for termination.
 */
void pi_cl_hyperram_free(struct pi_device *device, uint32_t chunk, uint32_t size, pi_cl_hyperram_free_req_t *req);

/** \brief Wait until the specified hyperram alloc request has finished.
 *
 * This blocks the calling core until the specified cluster hyperbus allocation is finished.
 *
 * \param req       The request structure used for termination.
 * \return NULL     if not enough memory was available, otherwise the address of the allocated chunk
 */
static inline uint32_t pi_cl_hyperram_alloc_wait(pi_cl_hyperram_alloc_req_t *req);

/** \brief Wait until the specified hyperbus free request has finished.
 *
 * This blocks the calling core until the specified cluster hyperbus free is finished.
 *
 * \param req       The request structure used for termination.
 * \return 0        if the operation was successful, -1 otherwise
 */
static inline void pi_cl_hyperram_free_wait(pi_cl_hyperram_free_req_t *req);


/** \brief Enqueue a sector erase command to the Hyperbus Flash.
 *
 * This command will erase a sector in order to program the Hyperflash.
 * The caller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip.
 * \param hyper_addr  The address of the sector in the Hyperbus.
 */
void pi_hyper_flash_erase(struct pi_device *device, uint32_t hyper_addr);

/** \brief Enqueue a program command to the Hyperbus Flash.
 *
 * This command will write the content of the given buffer inside the Hyperflash.
 * The caller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip.
 * \param hyper_addr  The address of the sector in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 */
void pi_hyper_flash_write(struct pi_device *device, uint32_t hyper_addr,
                          void *addr, uint32_t size);

/** \brief Enqueue a read to the Hyperbus Flash.
 *
 * This command will read from the Hyperflash to the buffer.
 * The caller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the Hyperbus chip.
 * \param hyper_addr  The address of the sector in the Hyperbus.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 */
void pi_hyper_flash_read(struct pi_device *device, uint32_t hyper_addr,
                         void *addr, uint32_t size);

/** \brief Synchronize programming/erase commands to the Hyperflash.
 *
 * This function synchronizes the programming and erase sequences, it waits the end
 * of previous command.
 *
 * \param device      The device descriptor of the Hyperbus chip.
 */
void pi_hyper_flash_sync(struct pi_device *device);

//!@}

/**
 * @} end of Hyperbus
 */

#endif  /* __PI_DRIVERS_HYPERBUS_H__ */
