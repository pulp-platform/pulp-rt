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

#ifndef __MC_MC_HYPERRAM_H__
#define __MC_MC_HYPERRAM_H__




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

/** \struct mc_hyperram_conf_t
 * \brief HyperRAM configuration structure.
 *
 * This structure is used to pass the desired HyperRAM configuration to the runtime when opening the device.
 */
struct mc_hyperram_conf 
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
typedef struct cl_hyperram_req_s cl_hyperram_req_t;

typedef struct cl_hyperram_alloc_req_s cl_hyperram_alloc_req_t;

typedef struct cl_hyperram_free_req_s cl_hyperram_free_req_t;



/** \brief Initialize an HyperRAM configuration with default values.
 *
 * The structure containing the configuration must be kept alive until the camera is opened.
 * Can only be called from fabric-controller side.
 *
 * \param conf A pointer to the HyperRAM configuration.
 */
void mc_hyperram_conf_init(struct mc_hyperram_conf *conf);



/** \brief Open an HyperRAM device.
 *
 * This function must be called before the HyperRAM device can be used. It will do all the needed configuration to make it
 * usable and also return a handle used to refer to this opened device when calling other functions.
 * This operation is asynchronous and its termination can be managed through an event.
 * Can only be called from fabric-controller side.
 *
 * \param dev_name  The device name. This name should correspond to the one used to configure the devices managed by the runtime.
 * \param conf      A pointer to the HyperRAM configuration. Can be NULL to take default configuration.
 * \param event     The event used for managing termination.
 * \return          NULL if the device is not found, or a handle identifying the device and which can be used with other functions
 */
int mc_hyperram_open_with_conf(struct pmsis_device *device, void *conf);



/** \brief Close an opened HyperRAM device.
 *
 * This function can be called to close an opened HyperRAM device once it is not needed anymore, in order to free
 * all allocated resources. Once this function is called, the device is not accessible anymore and must be opened
 * again before being used.
 * This operation is asynchronous and its termination can be managed through an event.
 * Can only be called from fabric-controller side.
 *
 * \param handle    The handler of the device which was returned when the device was opened.
 * \param event     The event used for managing termination.
 */
void mc_hyperram_close(struct pmsis_device *device);





/** \brief Enqueue a read copy to the HyperRAM (from HyperRAM to processor).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * An event can be specified in order to be notified when the transfer is finished.
 * Can only be called from fabric-controller side.
 *
 * \param dev         The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param event       The event used to notify the end of transfer. See the documentation of mc_event_t for more details.
 */
void mc_hyperram_read(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size);

void mc_hyperram_read_async(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, struct fc_task *task);



/** \brief Enqueue a write copy to the HyperRAM (from processor to HyperRAM).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * An event can be specified in order to be notified when the transfer is finished.
 * Can only be called from fabric-controller side.
 *
 * \param dev         The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param event       The event used to notify the end of transfer. See the documentation of mc_event_t for more details.
 */
void mc_hyperram_write(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size);

void mc_hyperram_write_async(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, struct fc_task *task);



/** \brief Enqueue a 2D read copy (rectangle area) to the HyperRAM (from HyperRAM to processor).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * An event can be specified in order to be notified when the transfer is finished.
 * Can only be called from fabric-controller side.
 *
 * \param dev         The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one. Must fit in 16 bits, i.e. must be less than 65536.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line. Must fit in 16 bits, i.e. must be less than 65536.
 * \param event       The event used to notify the end of transfer. See the documentation of mc_event_t for more details.
 */
static inline void mc_hyperram_read_2d(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, uint32_t stride, uint32_t length);

static inline void mc_hyperram_read_2d_async(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, uint32_t stride, uint32_t length, struct fc_task *task);



/** \brief Enqueue a 2D write copy (rectangle area) to the HyperRAM (from processor to HyperRAM).
 *
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * An event can be specified in order to be notified when the transfer is finished.
 * Can only be called from fabric-controller side.
 *
 * \param dev         The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param stride      2D stride, which is the number of bytes which are added to the beginning of the current line to switch to the next one. Must fit in 16 bits, i.e. must be less than 65536.
 * \param length      2D length, which is the number of transfered bytes after which the driver will switch to the next line. Must fit in 16 bits, i.e. must be less than 65536.
 * \param event       The event used to notify the end of transfer. See the documentation of mc_event_t for more details.
 */
static inline void mc_hyperram_write_2d(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, uint32_t stride, uint32_t length);

static inline void mc_hyperram_write_2d_async(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, uint32_t stride, uint32_t length, struct fc_task *task);



/** \brief Allocate HyperRAM memory
 *
 * The allocated memory is 4-bytes aligned. The allocator uses some meta-data stored in the fabric controller memory
 * for every allocation so it is advisable to do as few allocations as possible to lower the memory overhead.
 * Can only be called from fabric-controller side.
 *
 * \param dev    The device descriptor of the HyperRAM chip for which the memory must be allocated
 * \param size   The size in bytes of the memory to allocate
 * \return NULL if not enough memory was available, otherwise the address of the allocated chunk
 */
uint32_t mc_hyperram_alloc(struct pmsis_device *device, uint32_t size);

/** \brief Free HyperRAM memory
 *
 * The allocator does not store any information about the allocated chunks, thus the size of the allocated
 * chunk to to be freed must be provided by the caller.
 * Can only be called from fabric-controller side.
 *
 * \param dev    The device descriptor of the HyperRAM chip for which the memory must be freed
 * \param chunk  The allocated chunk to free
 * \param size   The size in bytes of the memory chunk which was allocated
 * \return 0 if the operation was successful, -1 otherwise
 */
int mc_hyperram_free(struct pmsis_device *device, uint32_t chunk, uint32_t size);



/** \brief Enqueue a read copy to the HyperRAM from cluster side (from HyperRAM to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam read copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param dev         The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void cl_hyperram_cluster_read(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, cl_hyperram_req_t *req);



/** \brief Enqueue a write copy to the HyperRAM from cluster side (from HyperRAM to processor).
 *
 * This function is a remote call that the cluster can do to the fabric-controller in order to ask
 * for an HyperRam write copy.
 * The copy will make an asynchronous transfer between the HyperRAM and one of the processor memory areas.
 * A pointer to a request structure must be provided so that the runtime can properly do the remote call.
 * Can only be called from cluster side.
 *
 * \param dev         The device descriptor of the HyperRAM chip on which to do the copy.
 * \param addr        The address of the copy in the processor.
 * \param hyper_addr  The address of the copy in the HyperRAM.
 * \param size        The size in bytes of the copy
 * \param req         A pointer to the HyperRam request structure. It must be allocated by the caller and kept alive until the copy is finished.
 */
static inline void cl_hyperram_cluster_write(struct pmsis_device *device,
  void *addr, uint32_t hyper_addr, uint32_t size, cl_hyperram_req_t *req);



/** \brief Wait until the specified hyperram request has finished.
 *
 * This blocks the calling core until the specified cluster remote copy is finished.
 *
 * \param req       The request structure used for termination.
 */
static inline void cl_hyperram_cluster_write_wait(cl_hyperram_req_t *req);

static inline void cl_hyperram_cluster_read_wait(cl_hyperram_req_t *req);

/** \brief Allocate HyperRAM memory from cluster
 *
 * The allocated memory is 4-bytes aligned. The allocator uses some meta-data stored in the fabric controller memory
 * for every allocation so it is advisable to do as few allocations as possible to lower the memory overhead.
 *
 * \param dev    The device descriptor of the HyperRAM chip for which the memory must be allocated
 * \param size   The size in bytes of the memory to allocate
 * \param req    The request structure used for termination.
 */
void cl_hyperram_alloc_cluster(struct pmsis_device *device, uint32_t size, cl_hyperram_alloc_req_t *req);

/** \brief Free HyperRAM memory from cluster
 *
 * The allocator does not store any information about the allocated chunks, thus the size of the allocated
 * chunk to to be freed must be provided by the caller.
 * Can only be called from fabric-controller side.
 *
 * \param dev    The device descriptor of the HyperRAM chip for which the memory must be freed
 * \param chunk  The allocated chunk to free
 * \param size   The size in bytes of the memory chunk which was allocated
 * \param req    The request structure used for termination.
 */
void cl_hyperram_free_cluster(struct pmsis_device *device, uint32_t chunk, uint32_t size, cl_hyperram_free_req_t *req);

/** \brief Wait until the specified hyperram alloc request has finished.
 *
 * This blocks the calling core until the specified cluster hyperram allocation is finished.
 *
 * \param req       The request structure used for termination.
 * \return NULL     if not enough memory was available, otherwise the address of the allocated chunk
 */
static inline uint32_t cl_hyperram_alloc_cluster_wait(cl_hyperram_alloc_req_t *req);

/** \brief Wait until the specified hyperram free request has finished.
 *
 * This blocks the calling core until the specified cluster hyperram free is finished.
 *
 * \param req       The request structure used for termination.
 * \return 0        if the operation was successful, -1 otherwise
 */
static inline void cl_hyperram_free_cluster_wait(cl_hyperram_free_req_t *req);

//!@}

/**
 * @} end of HyperRAM
 */



/// @cond IMPLEM

struct cl_hyperram_req_s {
  struct pmsis_device *device;
  void *addr;
  uint32_t hyper_addr;
  uint32_t size;
  int32_t stride;
  uint32_t length;
  rt_event_t event;
  int done;
  unsigned char cid;
  unsigned char is_write;
  unsigned char is_2d;
};

struct cl_hyperram_alloc_req_s {
  struct pmsis_device *device;
  uint32_t result;
  uint32_t  size;
  rt_event_t event;
  char done;
  char cid;
};

struct cl_hyperram_free_req_s {
  struct pmsis_device *device;
  uint32_t result;
  uint32_t size;
  uint32_t chunk;
  rt_event_t event;
  char done;
  char cid;
};

#if defined(ARCHI_HAS_CLUSTER)

static inline uint32_t cl_hyperram_alloc_cluster_wait(cl_hyperram_alloc_req_t *req)
{
	while((*(volatile char *)&req->done) == 0)
	{
		eu_evt_maskWaitAndClr(1<<RT_CLUSTER_CALL_EVT);
	}
	return req->result;
}

static inline void cl_hyperram_free_cluster_wait(cl_hyperram_free_req_t *req)
{
	while((*(volatile char *)&req->done) == 0)
	{
		eu_evt_maskWaitAndClr(1<<RT_CLUSTER_CALL_EVT);
	}
}

static inline __attribute__((always_inline)) void cl_hyperram_cluster_wait(cl_hyperram_req_t *req)
{
  while((*(volatile int *)&req->done) == 0)
  {
    eu_evt_maskWaitAndClr(1<<RT_CLUSTER_CALL_EVT);
  }
}

#endif


/// @endcond

#endif
