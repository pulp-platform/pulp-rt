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
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "rt/rt_data.h"
#include "archi/pulp.h"
#if UDMA_VERSION == 2
#include "archi/udma/udma_v2.h"
#else
#include "archi/udma/udma_v3.h"
#endif
#if SOC_EU_VERSION == 2
#include "archi/soc_eu/soc_eu_v2.h"
#else
#include "archi/soc_eu/soc_eu_v1.h"
#endif


  // x8: periph, x10:event, x8,x11,x12:temp
  .global __rt_spim_handle_eot
__rt_spim_handle_eot:

  lw     x12, RT_PERIPH_SPIM_T_PENDING1(x8)
  lw     x10, RT_PERIPH_SPIM_T_PENDING0(x8)
  sw     x12, RT_PERIPH_SPIM_T_PENDING0(x8)

  lw     x11, RT_PERIPH_COPY_T_EVENT(x10)

  la     x9, __rt_fc_socevents_handler_exit
  bne    x11, zero, __rt_event_enqueue
