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
 * Authors: Eric Flamand, GreenWaves Technologies (eric.flamand@greenwaves-technologies.com)
 *          Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "rt/rt_api.h"

int __rt_freq_domains[RT_FREQ_NB_DOMAIN];

void rt_freq_wait_convergence(int fll)
{
}

int rt_freq_set_and_get(rt_freq_domain_e domain, unsigned int freq, unsigned int *out_freq)
{
  int irq = rt_irq_disable();

  rt_trace(RT_TRACE_FREQ, "Setting domain frequency (domain: %d, freq: %d)\n", domain, freq);

  int fll_freq = __rt_fll_set_freq(domain, freq);
  if (out_freq)
    *out_freq = fll_freq;

  __rt_freq_domains[domain] = freq;

  rt_irq_restore(irq);

  return 0;
}

void __rt_freq_init()
{
  __rt_flls_constructor();

  __rt_freq_domains[RT_FREQ_DOMAIN_FC] = __rt_fll_init(__RT_FLL_FC);

  __rt_freq_domains[RT_FREQ_DOMAIN_PERIPH] = __rt_fll_init(__RT_FLL_PERIPH);

  __rt_freq_domains[RT_FREQ_DOMAIN_CL] = __rt_fll_init(__RT_FLL_CL);

}
