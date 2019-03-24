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

/* 
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "rt/rt_api.h"
#include "stdio.h"


#define RT_PMU_MRAM_ID    0
#define RT_PMU_CLUSTER_ID 1

#define RT_PMU_STATE_OFF 0
#define RT_PMU_STATE_ON  1

/* Maestro internal events */
#define MAESTRO_EVENT_ICU_OK            (1<<0)
#define MAESTRO_EVENT_ICU_DELAYED       (1<<1)
#define MAESTRO_EVENT_MODE_CHANGED      (1<<2)
#define MAESTRO_EVENT_PICL_OK           (1<<3)
#define MAESTRO_EVENT_SCU_OK            (1<<4)

RT_L2_RET_DATA static uint32_t __rt_pmu_domains_on;
RT_L2_RET_DATA static uint32_t __rt_pmu_domains_on_pending;

RT_FC_DATA static rt_event_t *__rt_pmu_pending_requests;
RT_FC_DATA static rt_event_t *__rt_pmu_pending_requests_tail;



static inline __attribute__((always_inline)) void __rt_pmu_apply_state(int domain, int state)
{
  __rt_pmu_domains_on_pending = (__rt_pmu_domains_on & ~(1 << domain)) | (state << domain);

  maestro_trigger_sequence(1 << __rt_pmu_domains_on_pending);
}



static void __attribute__((interrupt)) __rt_pmu_scu_handler()
{
  //   Clear PICL_OK interrupt
  PMU_WRITE(MAESTRO_DLC_IFR_OFFSET, MAESTRO_EVENT_SCU_OK);

  __rt_pmu_domains_on = __rt_pmu_domains_on_pending;

  rt_event_t *event = __rt_pmu_pending_requests;
  if (event)
  {
    __rt_pmu_pending_requests = event->next;

    __rt_pmu_apply_state(event->data[0], event->data[1]);

    if (event->data[0] == RT_PMU_CLUSTER_ID && event->data[1] == RT_PMU_STATE_ON)
    {
      // Temporary workaround until HW bug on cluster isolation is fixed
      apb_soc_cl_isolate_set(ARCHI_APB_SOC_CTRL_ADDR, 0);
    }

    __rt_push_event(event->sched, event);
  }
}



static void __rt_pmu_change_domain_power(rt_event_t *event, int *pending, int domain, int state)
{
  if (__rt_pmu_domains_on_pending == __rt_pmu_domains_on)
  {
    __rt_pmu_apply_state(domain, state);
  }
  else
  {
    event->data[0] = domain;
    event->data[1] = state;

    if (__rt_pmu_pending_requests == NULL)
      __rt_pmu_pending_requests = event;
    else
      __rt_pmu_pending_requests_tail->next = event;

    __rt_pmu_pending_requests_tail = event;
    event->next = NULL;


    *pending = 1;
  }
}



void __rt_pmu_cluster_power_down(rt_event_t *event, int *pending)
{
  // Temporary workaround until HW bug on cluster isolation is fixed
  apb_soc_cl_isolate_set(ARCHI_APB_SOC_CTRL_ADDR, 1);

  __rt_pmu_change_domain_power(event, pending, RT_PMU_CLUSTER_ID, RT_PMU_STATE_OFF);
}



int __rt_pmu_cluster_power_up(rt_event_t *event, int *pending)
{
  __rt_pmu_change_domain_power(event, pending, RT_PMU_CLUSTER_ID, RT_PMU_STATE_ON);

  if (*pending == 0)
  {
    // Temporary workaround until HW bug on cluster isolation is fixed
    apb_soc_cl_isolate_set(ARCHI_APB_SOC_CTRL_ADDR, 0);
  }

  return 1;
}



void __rt_pmu_init()
{
  // At startup, everything is off.
  // TODO once wakeup is supported, see if we keep some domains on
  __rt_pmu_domains_on = 0;
  __rt_pmu_domains_on_pending = 0;
  __rt_pmu_pending_requests = NULL;

  // Activate SCU handler, it will be called every time a sequence is
  // finished to clear the interrupt in Maestro
  rt_irq_set_handler(ARCHI_FC_EVT_SCU_OK, __rt_pmu_scu_handler);
  rt_irq_mask_set(1<<ARCHI_FC_EVT_SCU_OK);

  // Disable all Maestro interrupts but PICL_OK and SCU_OK
  PMU_WRITE(MAESTRO_DLC_IMR_OFFSET, 0x7);
}
