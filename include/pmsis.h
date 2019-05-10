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

#ifndef __PMSIS__H__
#define __PMSIS__H__

#ifdef __RT_USE_PMSIS__

#include "pmsis_cluster/cluster_sync/fc_to_cl_delegate.h"

#else

struct pmsis_device;

// device struct, it wont stay here
typedef struct pmsis_device {
    struct pmsis_device_api *api; // function pointers
    void *config; // driver conf: might be filled either using device tree or manually
    void *data; // driver data
} pmsis_device_t;

typedef struct cluster_driver_conf {
    int id;
    void *heap_start;
    uint32_t heap_size;
} cluster_driver_conf_t;

struct cluster_task{
    // entry function and its argument(s)
    void (*entry)(void*);
    void *arg;
    // pointer to first stack, and size for each cores
    void *stacks;
    uint32_t stack_size;
    uint32_t _slave_stack_size;
    // mask of cores to be activated
    int core_mask;
    // callback called at task completion
    struct fc_task_t *completion_callback;
    int stack_allocated;
    // to implement a fifo
    struct cluster_task *next;
};

#endif

#endif

