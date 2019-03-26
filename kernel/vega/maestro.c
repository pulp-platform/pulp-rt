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

#define COLD_BOOT       0     /* SoC cold boot, from Flash usually */
#define DEEP_SLEEP_BOOT 1     /* Reboot from deep sleep state, state has been lost, somehow equivalent to COLD_BOOT */
#define RETENTIVE_BOOT  2     /* Reboot from Retentive state, state has been retained, should bypass flash reload */


#define RT_PMU_MRAM_ID    0
#define RT_PMU_CSI_ID     1
#define RT_PMU_CLUSTER_ID 2
#define RT_PMU_CHIP_ID    3

#define RT_PMU_STATE_OFF 0
#define RT_PMU_STATE_ON  1

#define RT_PMU_RETENTION_OFF 0
#define RT_PMU_RETENTION_ON  1

/* Maestro internal events */
#define MAESTRO_EVENT_ICU_OK            (1<<0)
#define MAESTRO_EVENT_ICU_DELAYED       (1<<1)
#define MAESTRO_EVENT_MODE_CHANGED      (1<<2)
#define MAESTRO_EVENT_PICL_OK           (1<<3)
#define MAESTRO_EVENT_SCU_OK            (1<<4)

// 1 if a sequence is pending, 0 otherwise
RT_L2_RET_DATA static uint32_t __rt_pmu_pending_sequence;

// 1 bit per domain, 1 means ON, 0 means OFF
RT_L2_RET_DATA static uint32_t __rt_pmu_domains_on;

// First pending sequence (waiting for current one to finish)
RT_FC_DATA static rt_event_t *__rt_pmu_pending_requests;
// Last pending sequence
RT_FC_DATA static rt_event_t *__rt_pmu_pending_requests_tail;



static inline __attribute__((always_inline)) void __rt_pmu_apply_state(int domain, int state, int ret)
{
  int sequence;

  // Update local domain status
  __rt_pmu_domains_on = (__rt_pmu_domains_on & ~(1 << domain)) | (state << domain);

  // Remember that a sequence is pending as no other one should be enqueued until
  // this one is finished.
  __rt_pmu_pending_sequence = 1;

  if (domain == RT_PMU_CHIP_ID)
  {
    sequence = 4 + ret;
  }
  else
  {
    sequence = domain*2 + 8 + state;
  }

  maestro_trigger_sequence(sequence);
}



static void __attribute__((interrupt)) __rt_pmu_scu_handler()
{
  // This handler gets called when a sequence is finished. It should then
  // acknoledge it and continue with the next pending sequence.

  // Clear end of sequence interrupt
  PMU_WRITE(MAESTRO_DLC_IFR_OFFSET, MAESTRO_EVENT_SCU_OK);

  // Notify that nothing is pending
  __rt_pmu_pending_sequence = 0;

  // And handle the next pending sequence
  rt_event_t *event = __rt_pmu_pending_requests;
  if (event)
  {
    __rt_pmu_pending_requests = event->next;

    __rt_pmu_apply_state(event->data[0], event->data[1], event->data[2]);

    __rt_push_event(event->sched, event);
  }
}



static void __rt_pmu_change_domain_power(rt_event_t *event, int *pending, int domain, int state, int retentive)
{
  // In case, no sequence is pending, just apply the new one and leave
  if (__rt_pmu_pending_sequence == 0)
  {
    // Note that this is asynchronous but we can notify the caller that it's done
    // as any access will be put on hold.
    // TODO this is probably not true for MRAM and CSI.
    __rt_pmu_apply_state(domain, state, retentive);
  }
  else
  {
    // Otherwise enqueue in the list of pending sequences and notify the caller
    // that the operation is pending.
    event->data[0] = domain;
    event->data[1] = state;
    event->data[2] = retentive;

    if (__rt_pmu_pending_requests == NULL)
      __rt_pmu_pending_requests = event;
    else
      __rt_pmu_pending_requests_tail->next = event;

    __rt_pmu_pending_requests_tail = event;
    event->next = NULL;

    if (pending)
      *pending = 1;
  }
}



void __rt_pmu_cluster_power_down(rt_event_t *event, int *pending)
{
  __rt_pmu_change_domain_power(event, pending, RT_PMU_CLUSTER_ID, RT_PMU_STATE_OFF, RT_PMU_RETENTION_OFF);
}



int __rt_pmu_cluster_power_up(rt_event_t *event, int *pending)
{
  __rt_pmu_change_domain_power(event, pending, RT_PMU_CLUSTER_ID, RT_PMU_STATE_ON, RT_PMU_RETENTION_OFF);

  return 1;
}




void rt_pm_wakeup_clear_all()
{
  // This function is called to clear all pending wakeup conditions.
  // They are all stored in the apb soc, reading SLEEP_CTRL will clear all of
  // them.
  apb_soc_safe_pmu_sleepctrl_get(ARCHI_APB_SOC_CTRL_ADDR);
}




void rt_pm_wakeup_gpio_conf(int active, int gpio, rt_pm_wakeup_gpio_mode_e mode)
{
#if 0
  // 6 -> 10 : gpio number which cam wakeup chip
  PMURetentionState.Fields.ExternalWakeUpSource = gpio;

  // 11 -> 12 : falling or raising edge
  PMURetentionState.Fields.ExternalWakeUpMode   = mode;

  // 13 : GPIO wakeup enabled
  PMURetentionState.Fields.ExternalWakeupEnable = active;

  // Write to APB SOC SLEEP_CTRL
  SetRetentiveState(PMURetentionState.Raw);
#endif
}



static void __attribute__((aligned(16))) __rt_pmu_shutdown(int retentive)
{
  int irq = rt_irq_disable();

  unsigned int boot_mode = retentive ? RETENTIVE_BOOT : DEEP_SLEEP_BOOT;

  // For now we just go to sleep and activate RTC wakeup with all
  // cuts retentive
  apb_soc_safe_pmu_sleepctrl_set(
    ARCHI_APB_SOC_CTRL_ADDR,
    APB_SOC_SAFE_PMU_SLEEPCTRL_REBOOT(boot_mode)  |
    APB_SOC_SAFE_PMU_SLEEPCTRL_RTCWAKE_EN(1)      |
    APB_SOC_SAFE_PMU_SLEEPCTRL_RET_MEM(0xffff)
  );

  // Change power state
  rt_event_t *event = __rt_wait_event_prepare_blocking();
  __rt_pmu_change_domain_power(event, NULL, RT_PMU_CHIP_ID, RT_PMU_STATE_OFF, retentive);

  // And wait for ever as we are supposed to go to sleep and start from main
  // after wakeup.
  while(1)
  {
    hal_itc_enable_value_set(0);
    hal_itc_wait_for_interrupt();
  }

  rt_irq_restore(irq);
}



int rt_pm_state_switch(rt_pm_state_e state, rt_pm_state_flags_e flags)
{
  if (state == RT_PM_STATE_DEEP_SLEEP)
  {
    if ((flags & RT_PM_STATE_FAST) == 0)
      return -1;

    __rt_pmu_shutdown(0);

    return 0;
  }
  else if (state == RT_PM_STATE_SLEEP)
  {
    if ((flags & RT_PM_STATE_FAST) == 0)
      return -1;
    
    __rt_pmu_shutdown(1);

    return 0;
  }

  return -1;
}



rt_pm_wakeup_e rt_pm_wakeup_state()
{
  // The wakeup state is a user information stored in the retentive
  // SLEEP_CtRL register just before going to sleep.
  apb_soc_safe_pmu_sleepctrl_t sleepctrl = {
    .raw = apb_soc_sleep_ctrl_get(ARCHI_APB_SOC_CTRL_ADDR)
  };

  if (sleepctrl.reboot == DEEP_SLEEP_BOOT)
    return RT_PM_WAKEUP_DEEPSLEEP;
  else if (sleepctrl.reboot == RETENTIVE_BOOT)
    return RT_PM_WAKEUP_SLEEP;

  return RT_PM_WAKEUP_COLD;
}



void __rt_pmu_init()
{
  // At startup, everything is off.
  // TODO once wakeup is supported, see if we keep some domains on
  __rt_pmu_domains_on = 1 << RT_PMU_CHIP_ID;
  __rt_pmu_pending_sequence = 0;
  __rt_pmu_pending_requests = NULL;

  // Activate SCU handler, it will be called every time a sequence is
  // finished to clear the interrupt in Maestro
  rt_irq_set_handler(ARCHI_FC_EVT_SCU_OK, __rt_pmu_scu_handler);
  rt_irq_mask_set(1<<ARCHI_FC_EVT_SCU_OK);

  // Disable all Maestro interrupts but PICL_OK and SCU_OK
  PMU_WRITE(MAESTRO_DLC_IMR_OFFSET, 0x7);
}
