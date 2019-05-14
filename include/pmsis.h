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

#include <rt/rt_api.h>

#include "pmsis_cluster/cluster_sync/fc_to_cl_delegate.h"


static inline struct cluster_task *mc_cluster_task(struct cluster_task *task, void (*entry)(void*), void *arg)
{
  task->entry = entry;
  task->arg = arg;
  task->stacks = NULL;
  task->stack_size = 0;
  task->nb_cores = 0;
  return task;
}

static inline struct fc_task *mc_task_callback(struct fc_task *task, void (*callback)(void*), void *arg)
{
  task->id = FC_TASK_CALLBACK_ID;
  task->arg[0] = (uint32_t)callback;
  task->arg[1] = (uint32_t)arg;
  return task;
}

static inline struct fc_task *mc_task(struct fc_task *task)
{
  task->id = FC_TASK_NONE_ID;
  task->arg[0] = (uint32_t)0;
  return task;
}


#endif

