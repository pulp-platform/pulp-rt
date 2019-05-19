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

#ifndef __PI_PI_HYPERRAM_H__
#define __PI_PI_HYPERRAM_H__




/**
* @ingroup groupDrivers
*/



/**
 * @defgroup HyperRAM HyperRAM
 *
 * The HyperRAM driver provides support for transferring data between an external HyperRAM chip
 * and the processor running this driver.
 *
 * The driver provides a memory allocator for allocating and freeing memory in the HyperRAM chip
 * and an asynchronous API for transferring data.
 *
 */

/**
 * @addtogroup HyperRAM
 * @{
 */

/**@{*/

/** \struct pi_hyperram_conf
 * \brief HyperRAM configuration structure.
 *
 * This structure is used to pass the desired HyperRAM configuration to the runtime when opening the device.
 */
struct pi_hyperram_conf 
{
  signed char id;         /*!< If it is different from -1, this specifies on which hyperbus interface the device is connected. */
  signed int ram_size;   /*!< Size of the ram. */
};

/** \brief HyperRAM request structure.
 *
 * This structure is used by the runtime to manage a cluster remote copy with the HyperRAM.
 * It must be instantiated once for each copy and must be kept alive until the copy is finished.
 * It can be instantiated as a normal variable, for example as a global variable, a local one on the stack,
 * or through a memory allocator.
 */
typedef struct pi_cl_hyperram_req_s pi_cl_hyperram_req_t;

/** \brief HyperRAM memory allocation request structure.
 *
 * This structure is used by the runtime to manage a cluster remote memory allocation
 * from the HyperRAM.
 * It must be instantiated once for each on-going allocation and must be kept alive
 * until the allocation is finished.
 * It can be instantiated as a normal variable, for example as a global variable, a local one on the stack,
 * or through a memory allocator.
 */
typedef struct pi_cl_hyperram_alloc_req_s pi_cl_hyperram_alloc_req_t;

/** \brief HyperRAM memory free request structure.
 *
 * This structure is used by the runtime to manage a cluster remote memory free
 * from the HyperRAM.
 * It must be instantiated once for each on-going free and must be kept alive
 * until the free is finished.
 * It can be instantiated as a normal variable, for example as a global variable, a local one on the stack,
 * or through a memory allocator.
 */
typedef struct pi_cl_hyperram_free_req_s pi_cl_hyperram_free_req_t;

/** \brief Initialize an HyperRAM configuration with default values.
 *
 * The structure containing the configuration must be kept alive until the camera is opened.
 *
 * \param conf A pointer to the HyperRAM configuration.
 */
void pi_hyperram_conf_init(struct pi_hyperram_conf *conf);

/** \brief Open an HyperRAM device.
 *
 * This function must be called before the HyperRAM device can be used. It will do all the needed configuration to make it
 * usable and also return a handle used to refer to this opened device when calling other functions.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param device    The device structure of the device to open.
 * \return          0 if the operation is successfull, -1 if there was an error
 */
int pi_hyperram_open(struct pi_device *device);

/** \brief Close an opened HyperRAM device.
 *
 * This function can be called to close an opened HyperRAM device once it is not needed anymore, in order to free
 * all allocated resources. Once this function is called, the device is not accessible anymore and must be opened
 * again before being used.
 * This operation is asynchronous and its termination can be managed through an event.
 *
 * \param device    The device structure of the device to close.
 */
void pi_hyperram_close(struct pi_device *device);

/** \brief Enqueue a read copy to the HyperRAM (from HyperRAM to processor).
 *
 * The copy will make a transfer between the HyperRAM and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 */
void pi_hyperram_read(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size);

/** \brief Enqueue an asynchronous read copy to the HyperRAM (from HyperRAM to processor).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param task        The task used to notify the end of transfer. See the documentation of pi_fc_task for more details.
 */
void pi_hyperram_read_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, struct pi_fc_task *task);

/** \brief Enqueue a write copy to the HyperRAM (from processor to HyperRAM).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 */
void pi_hyperram_write(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size);

/** \brief Enqueue an asynchronous write copy to the HyperRAM (from processor to HyperRAM).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param task        The task used to notify the end of transfer. See the documentation of pi_fc_task for more details.
 */
void pi_hyperram_write_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, struct pi_fc_task *task);

/** \brief Enqueue a 2D read copy (rectangle area) to the HyperRAM (from HyperRAM to processor).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 */
void pi_hyperram_read_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length);

/** \brief Enqueue an asynchronous 2D read copy (rectangle area) to the HyperRAM (from HyperRAM to processor).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_fc_task for more details.
 */
void pi_hyperram_read_2d_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, struct pi_fc_task *task);

/** \brief Enqueue a 2D write copy (rectangle area) to the HyperRAM (from processor to HyperRAM).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * The calller is blocked until the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 */
void pi_hyperram_write_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length);

/** \brief Enqueue an asynchronous 2D write copy (rectangle area) to the HyperRAM (from processor to HyperRAM).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A task can be specified in order to be notified when the transfer is finished.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param task        The task used to notify the end of transfer. See the documentation of pi_fc_task for more details.
 */
void pi_hyperram_write_2d_async(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, struct pi_fc_task *task);




/** \brief Allocate HyperRAM memory
 *
 * The allocated memory is 4-bytes aligned. The allocator uses some meta-data stored in the fabric controller memory
 * for every allocation so it is advisable to do as few allocations as possible to lower the memory overhead.
 *
 * \param device The device descriptor of the HyperRAM chip for which the memory must be allocated
 * \param size   The size in bytes of the memory to allocate
 * \return NULL if not enough memory was available, otherwise the address of the allocated chunk
 */
uint32_t pi_hyperram_alloc(struct pi_device *device, uint32_t size);

/** \brief Free HyperRAM memory
 *
 * The allocator does not store any information about the allocated chunks, thus the size of the allocated
 * chunk to to be freed must be provided by the caller.
 *
 * \param device The device descriptor of the HyperRAM chip for which the memory must be freed
 * \param chunk  The allocated chunk to free
 * \param size   The size in bytes of the memory chunk which was allocated
 * \return 0 if the operation was successful, -1 otherwise
 */
int pi_hyperram_free(struct pi_device *device, uint32_t chunk, uint32_t size);

/** \brief Enqueue a read copy to the HyperRAM from cluster side (from HyperRAM to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam read copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyperram_read(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, pi_cl_hyperram_req_t *req);

/** \brief Enqueue a 2D read copy (rectangle area) to the HyperRAM from cluster side (from HyperRAM to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam read copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyperram_read_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, pi_cl_hyperram_req_t *req);

/** \brief Wait until the specified hyperram request has finished.
 *
 * This blocks the calling core until the specified cluster remote copy is finished.
 *
 * \param req       The request structure used for termination.
 */
static inline void pi_cl_hyperram_read_wait(pi_cl_hyperram_req_t *req);

/** \brief Enqueue a write copy to the HyperRAM from cluster side (from HyperRAM to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam write copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyperram_write(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, pi_cl_hyperram_req_t *req);

/** \brief Enqueue a 2D write copy (rectangle area) to the HyperRAM from cluster side (from HyperRAM to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam write copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyperram_write_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, pi_cl_hyperram_req_t *req);

/** \brief Wait until the specified hyperram request has finished.
 *
 * This blocks the calling core until the specified cluster remote copy is finished.
 *
 * \param req       The request structure used for termination.
 */
static inline void pi_cl_hyperram_write_wait(pi_cl_hyperram_req_t *req);

/** \brief Enqueue a copy with the HyperRAM from cluster side.
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param ext2loc     1 if the copy is from HyperRam to the chip or 0 for the contrary.
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyperram_copy(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, int ext2loc, pi_cl_hyperram_req_t *req);

/** \brief Enqueue a 2D copy (rectangle area) with the HyperRAM from cluster side.
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param device      The device descriptor of the HyperRAM chip on which to do the copy.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param addr        The address of the copy in the processor.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line.
 * \param ext2loc     1 if the copy is from HyperRam to the chip or 0 for the contrary.
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void pi_cl_hyperram_copy_2d(struct pi_device *device,
  uint32_t hyper_addr, void *addr, uint32_t size, uint32_t stride, uint32_t length, int ext2loc, pi_cl_hyperram_req_t *req);

/** \brief Allocate HyperRAM memory from cluster
 *
 * The allocated memory is 4-bytes aligned. The allocator uses some meta-data stored in the fabric controller memory
 * for every allocation so it is advisable to do as few allocations as possible to lower the memory overhead.
 *
 * \param device The device descriptor of the HyperRAM chip for which the memory must be allocated
 * \param size   The size in bytes of the memory to allocate
 * \param req    The request structure used for termination.
 */
void pi_cl_hyperram_alloc(struct pi_device *device, uint32_t size, pi_cl_hyperram_alloc_req_t *req);

/** \brief Free HyperRAM memory from cluster
 *
 * The allocator does not store any information about the allocated chunks, thus the size of the allocated
 * chunk to to be freed must be provided by the caller.
 * Can only be called from fabric-controller side.
 *
 * \param device The device descriptor of the HyperRAM chip for which the memory must be freed
 * \param chunk  The allocated chunk to free
 * \param size   The size in bytes of the memory chunk which was allocated
 * \param req    The request structure used for termination.
 */
void pi_cl_hyperram_free(struct pi_device *device, uint32_t chunk, uint32_t size, pi_cl_hyperram_free_req_t *req);

/** \brief Wait until the specified hyperram alloc request has finished.
 *
 * This blocks the calling core until the specified cluster hyperram allocation is finished.
 *
 * \param req       The request structure used for termination.
 * \return NULL     if not enough memory was available, otherwise the address of the allocated chunk
 */
static inline uint32_t pi_cl_hyperram_alloc_wait(pi_cl_hyperram_alloc_req_t *req);

/** \brief Wait until the specified hyperram free request has finished.
 *
 * This blocks the calling core until the specified cluster hyperram free is finished.
 *
 * \param req       The request structure used for termination.
 * \return 0        if the operation was successful, -1 otherwise
 */
static inline void pi_cl_hyperram_free_wait(pi_cl_hyperram_free_req_t *req);

//!@}

/**
 * @} end of HyperRAM
 */

#endif
