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

/* 
 * Authors: Eric Flamand, GreenWaves Technologies (eric.flamand@greenwaves-technologies.com)
 *          Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "rt/rt_api.h"
#include "stdio.h"

#define PMU_PICL_PACK(chipsel,addr) (((chipsel) << 5) | (addr))
#define PMU_DLC_PACK(state,picl) (((state) << 16) | ((picl) << 1) | 0x1) //write 0x2 at picl_reg

static inline void __rt_wait_for_event(unsigned int mask) {
#if defined(ITC_VERSION)
  hal_itc_wait_for_event_noirq(mask);
#else
  eu_evt_maskWaitAndClr(mask);
#endif
}

void __rt_pmu_cluster_power_down()
{
  //plp_trace(RT_TRACE_PMU, "Cluster power down\n");

  // Check bit 14 of bypass register to see if an external tool (like gdb) is preventing us
  // from shutting down the cluster
  if ((hal_pmu_bypass_get() >> APB_SOC_BYPASS_USER1_BIT) & 1) return;

  // Wait until cluster is not busy anymore as isolating it while
  // AXI transactions are sent would break everything
  // This part does not need to be done asynchronously as the caller is supposed to make 
  // sure the cluster is not active anymore..
  while (apb_soc_busy_get()) {
    __rt_wait_for_event(1<<ARCHI_FC_EVT_CLUSTER_NOT_BUSY);
  }

  // Block transactions from dc fifos to soc
  apb_soc_cluster_isolate_set(1);

  // Cluster clock-gating
  hal_pmu_bypass_set( (1<<ARCHI_PMU_BYPASS_ENABLE_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_POWER_BIT) );
  __rt_wait_for_event(1<<ARCHI_FC_EVT_CLUSTER_CG_OK);

  // Cluster shutdown
  hal_pmu_bypass_set( (1<<ARCHI_PMU_BYPASS_ENABLE_BIT) );
  __rt_wait_for_event(1<<ARCHI_FC_EVT_CLUSTER_POK);
  // We should not need to wait for power off as it is really quick but we actually do
}

int __rt_pmu_cluster_power_up()
{
  //plp_trace(RT_TRACE_PMU, "Cluster power up\n");

  /* Turn on power */
  hal_pmu_bypass_set( (1<<ARCHI_PMU_BYPASS_ENABLE_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_POWER_BIT) );
  __rt_wait_for_event(1<<ARCHI_FC_EVT_CLUSTER_POK);

  /* Clock ungate cluster */
  hal_pmu_bypass_set( (1<<ARCHI_PMU_BYPASS_ENABLE_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_POWER_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_RESET_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_CLOCK_BIT) );
  __rt_wait_for_event(1<<ARCHI_FC_EVT_CLUSTER_CG_OK);

  // Unblock transactions from dc fifos to soc
  apb_soc_cluster_isolate_set(0);

  // Tell external loader (such as gdb) that the cluster is on so that it can take it
  // into account
  hal_pmu_bypass_set( (1<<ARCHI_PMU_BYPASS_ENABLE_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_POWER_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_RESET_BIT) | (1<<ARCHI_PMU_BYPASS_CLUSTER_CLOCK_BIT) | (1 << APB_SOC_BYPASS_USER0_BIT));

  return 1;
}

static void __rt_pm_shutdown(int retentive)
{
  int irq = rt_irq_disable();
  int interrupt;

  // Notify the bridge that the chip is going to be inaccessible.
  // We don't do anything until we know that the bridge received the
  // notification to avoid any race condition.
  __rt_bridge_req_shutdown();

  apb_soc_sleep_control_t sleep_ctrl = {
    .raw=apb_soc_sleep_control_get(ARCHI_APB_SOC_CTRL_ADDR)
  };

  sleep_ctrl.mem_ret_0 = -1;
  sleep_ctrl.mem_ret_0 = -1;

  if (retentive) {
    sleep_ctrl.boot_type = RT_PM_WAKEUP_SLEEP;
    interrupt = 1;
  } else {
    sleep_ctrl.boot_type = RT_PM_WAKEUP_DEEPSLEEP;
    interrupt = 0;
  }
  sleep_ctrl.wakeup = 0; // Always start in nominal voltage for now
  sleep_ctrl.cluster_wakeup = 0;

  apb_soc_sleep_control_set(ARCHI_APB_SOC_CTRL_ADDR, sleep_ctrl.raw);

  maestro_dlc_pctrl_set(ARCHI_PMU_ADDR, PMU_DLC_PACK(1<<interrupt, PMU_PICL_PACK(MAESTRO_WIU_OFFSET, MAESTRO_WIU_IFR_1_OFFSET)));

  hal_itc_enable_value_set(0);
  while(1)
  {
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

    __rt_pm_shutdown(0);

    return 0;
  }
  else if (state == RT_PM_STATE_SLEEP)
  {
    if ((flags & RT_PM_STATE_FAST) == 0)
      return -1;
    
    __rt_pm_shutdown(1);

    return 0;
  }

  return -1;
}



rt_pm_wakeup_e rt_pm_wakeup_state()
{
  apb_soc_sleep_control_t sleep_ctrl = {
    .raw=apb_soc_sleep_control_get(ARCHI_APB_SOC_CTRL_ADDR)
  };

  return sleep_ctrl.boot_type;
}
