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

#ifndef __RT_RT_IRQ_H__
#define __RT_RT_IRQ_H__

/// @cond IMPLEM

void __rt_irq_init();
void rt_irq_set_handler(int irq, void (*handler)());

static inline void rt_irq_mask_set(unsigned int mask);

static inline void rt_irq_mask_clr(unsigned int mask);


extern void __rt_fc_socevents_handler();

void rt_irq_mask_set(unsigned int mask)
{
#if defined(ITC_VERSION) && defined(EU_VERSION)
  if (rt_is_fc()) hal_itc_enable_set(mask);
  else eu_irq_maskSet(mask);
#elif defined(ITC_VERSION)
  hal_itc_enable_set(mask);
#elif defined(EU_VERSION)
  eu_irq_maskSet(mask);
#endif
}

void rt_irq_mask_clr(unsigned int mask)
{
#if defined(ITC_VERSION) && defined(EU_VERSION)
  if (rt_is_fc()) hal_itc_enable_clr(mask);
  else eu_irq_maskClr(mask);
#elif defined(ITC_VERSION)
  hal_itc_enable_clr(mask);
#elif defined(EU_VERSION)
  eu_irq_maskClr(mask);
#endif
}


/// @endcond

#endif
