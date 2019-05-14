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

#ifndef __FC_TO_CL_H__
#define __FC_TO_CL_H__

#include "pmsis_cluster/cl_pmsis_types.h"
#include "pmsis_types.h"
/**
 * @ingroup groupCluster
 */

/**
 * @defgroup clusterDriver
 *
 * Primitives to pilot the cluster
 * Can only be use from mcu side
 *
 */


/**
 * @addtogroup groupCluster
 * @{
 */

/**@{*/


/**
 * Defines for IOCTL
 */
#define SEND_TASK_ID 0
#define WAIT_FREE_ID 1
#define OPEN_ID 2
#define CLOSE_ID 3
#define SEND_TASK_ASYNC_ID 4
#define WAIT_FREE_ASYNC_ID 5
#define OPEN_ASYNC_ID 6
#define CLOSE_ASYNC_ID 7
/** \brief poweron the cluster (deactivate clock gating)
 * Calling this function will poweron the cluster
 * At the end of the call, cluster is ready to execute a task
 * Function is thread safe and reentrant
 * \param device initialized device structure
 * \param conf configuration struct for cluster driver
 */
int mc_cluster_open_with_conf(struct pmsis_device *device,
        void *conf);

/** \brief poweron the cluster (deactivate clock gating) - async version
 * Calling this function will poweron the cluster
 * At the end of the call, cluster is ready to execute a task
 * Function is thread safe and reentrant
 * \param cluster_id ID of the cluster to poweron
 * \param async_task asynchronous task to be executed at the end of operation
 */
int mc_cluster_open_with_conf_async(struct pmsis_device *device,
        void *conf, fc_task_t *async_task);

/** \brief send a task to the cluster
 * Calling this function will result in the cluster executing task passed as a parameter
 * Function is thread safe and reentrant
 * Will wait until cl_task has finished its execution
 * \param cluster_id ID of the cluster to execute task on
 * \param cl_task task structure containing task and its parameters
 */
int mc_cluster_send_task_to_cl(struct pmsis_device *device, struct cluster_task *task);

/** \brief send a task to the cluster - async version
 * Calling this function will result in the cluster executing task passed as a parameter
 * Function is thread safe and reentrant
 * Will not wait for end of cl_task execution, but will schedule async task to
 * be executed at cl_task end.
 * \param cluster_id ID of the cluster to execute task on
 * \param cl_task task structure containing task and its parameters
 * \param async_task structure to execute at the end of cl_task execution
 */
int mc_cluster_send_task_to_cl_async(struct pmsis_device *device, struct cluster_task *cl_task,
        fc_task_t *async_task);

/** \brief Wait for the cluster to be free i.e. nothing executes on it
 * Wait until no task is executed on cluster
 * \param cluster_id ID of the cluster to wait on
 */
void mc_cluster_wait_free(struct pmsis_device *device);

/** \brief Wait for the cluster to be free i.e. nothing executes on it
 * Wait until no task is executed on cluster
 * \param cluster_id ID of the cluster to wait on
 * \param async_task asynchronous task to be executed at the end of operation
 */
void mc_cluster_wait_free_async(struct pmsis_device *device, fc_task_t *async_task);

/** \brief poweroff the cluster (activate clock gating)
 * Stops the cluster and activate clock gating
 * will wait until cluster has finished executing all its tasks
 * If multiple threads are using the cluster, will only decrement a semaphore
 * \param cluster_id ID of the cluster to poweroff
 */
int mc_cluster_close(struct pmsis_device *device);

/** \brief poweroff the cluster (activate clock gating) - async version
 * Stops the cluster and activate clock gating
 * will wait until cluster has finished executing all its tasks
 * If multiple threads are using the cluster, will only decrement a semaphore
 * \param cluster_id ID of the cluster to poweroff
 * \param async_task asynchronous task to be executed at the end of operation
 */
int mc_cluster_close_async(struct pmsis_device *device, fc_task_t *async_task);

uint32_t mc_cluster_ioctl(struct pmsis_device *device, uint32_t func_id, void *arg);

uint32_t mc_cluster_ioctl_async(struct pmsis_device *device, uint32_t func_id,
        void *arg, fc_task_t *async_task);

/** \brief check if any cluster is on
 */
uint8_t mc_cluster_is_on(void);

// --- Useful defines to manipulate cluster objects
//
#define  GAP_CLUSTER_TINY_DATA(id, addr) (CLUSTER_BASE + 0x400000*(id) + (addr & 0xFFF))
#define  GAP_CLUSTER_CORE0_MASK 0x00000001
#define  GAP_CLUSTER_WITHOUT_CORE0_MASK           0xFFFFFFFE

//!@}

/**
 * @} end of Team group
 */
#endif
