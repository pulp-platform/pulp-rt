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

void __rt_pmu_cluster_power_down()
{
  if (rt_platform() == ARCHI_PLATFORM_FPGA)
  {
    // On the FPGA the only thing to manage is the cluster isolation
    PMU_IsolateCluster(1);
  }
}

void __rt_pmu_cluster_power_up()
{
  if (rt_platform() == ARCHI_PLATFORM_FPGA)
  {
    // On the FPGA the only thing to manage is the cluster isolation
    PMU_IsolateCluster(0);
  }
  else
  {
    PMU_BypassT Bypass;

    Bypass.Raw = GetPMUBypass();
    
    if (Bypass.Fields.ClusterClockGate == 0)
    {
      /* Clock gate FLL cluster */
      Bypass.Fields.ClusterClockGate = 1; SetPMUBypass(Bypass.Raw);

      /* Wait for clock gate done event */
      __rt_periph_wait_event(ARCHI_SOC_EVENT_ICU_DELAYED, 1);
    }

    /* Turn on power */
    Bypass.Fields.ClusterState = 1; SetPMUBypass(Bypass.Raw);

    /* Wait for TRC OK event */
    __rt_periph_wait_event(ARCHI_SOC_EVENT_CLUSTER_ON_OFF, 1);

    /* De assert Isolate on cluster */
    PMU_IsolateCluster(0);

    /* De Assert Reset cluster */
    Bypass.Fields.ClusterReset = 0; SetPMUBypass(Bypass.Raw);

    /* Clock ungate FLL cluster */
    Bypass.Fields.ClusterClockGate = 0; SetPMUBypass(Bypass.Raw);

    /* Wait for clock gate done event */
      __rt_periph_wait_event(ARCHI_SOC_EVENT_ICU_DELAYED, 1);

    /* Tell external loader (such as gdb) that the cluster is on so that it can take it into account */
    Bypass.Raw |= 1 << APB_SOC_BYPASS_USER0_BIT; SetPMUBypass(Bypass.Raw);
  }
}

static void __attribute__((constructor)) __rt_pmu_init()
{
  if (rt_platform() != ARCHI_PLATFORM_FPGA)
  {
    PMU_BypassT Bypass;
    Bypass.Raw = GetPMUBypass();
    Bypass.Fields.Bypass = 1; 
    Bypass.Fields.BypassClock = 1;
    SetPMUBypass(Bypass.Raw);

    /* Disable all Maestro interrupts but PICL_OK and SCU_OK */
    PMU_Write(DLC_IMR, 0x7);
    /* Clear PICL_OK and SCU_OK interrupts in case they are sat */
    PMU_Write(DLC_IFR, (MAESTRO_EVENT_PICL_OK|MAESTRO_EVENT_SCU_OK));
    
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_CLUSTER_ON_OFF);
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_MSP);
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_ICU_MODE_CHANGED);
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_ICU_OK);
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_ICU_DELAYED);
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_PICL_OK);
    soc_eu_fcEventMask_setEvent(ARCHI_SOC_EVENT_SCU_OK);
  }
}
