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

#include "rt/rt_api.h"

#if defined(ARCHI_HAS_FC)

static uint32_t timer_count;
rt_event_t *first_delayed = NULL;


static inline void __rt_time_conf_timer_default()
{
  // Configure the FC timer in 64 bits mode as it will be used as a common
  // timer for all virtual timers.
  // We also use the ref clock to make the frequency stable.
  hal_timer_conf(
    hal_timer_fc_addr(0, 1), PLP_TIMER_ACTIVE, PLP_TIMER_RESET_ENABLED,
    PLP_TIMER_IRQ_DISABLED, PLP_TIMER_IEM_DISABLED, PLP_TIMER_CMPCLR_DISABLED,
    PLP_TIMER_ONE_SHOT_DISABLED, PLP_TIMER_REFCLK_ENABLED,
    PLP_TIMER_PRESCALER_DISABLED, 0, PLP_TIMER_MODE_64_DISABLED
  );
}


static int __rt_time_poweroff(void *arg)
{
  // Remember the current timer count so that we can restore it
  // when the system is powered-on
  timer_count = hal_timer_count_get(hal_timer_fc_addr(0, 1));

  return 0;
}

static int __rt_time_poweron(void *arg)
{
  // Restore the timer count we saved before shutdown
  hal_timer_count_set(hal_timer_fc_addr(0, 1), timer_count);

  return 0;
}

unsigned long long rt_time_get_us()
{
  // Get 64 bit timer counter value and convert it to microseconds
  // as the timer input is connected to the ref clock.
  unsigned long long count = hal_timer_count_get(hal_timer_fc_addr(0, 1));
  if ((count >> 32) == 0) return count * 1000000 / ARCHI_REF_CLOCK;
  else return count / ARCHI_REF_CLOCK * 1000000;
}

void rt_event_push_delayed(rt_event_t *event, int us)
{
  int irq = rt_irq_disable();

  int set_irq = 0;
  rt_event_t *current = first_delayed, *prev=NULL;
  unsigned int ticks, ticks_from_now;
  uint32_t current_time = hal_timer_count_get(hal_timer_fc_addr(0, 1));
  
  // First compute the corresponding number of ticks.
  // The specified time is the minimum we must, so we have to round-up
  // the number of ticks.
  ticks = us / ( 1000000 / ARCHI_REF_CLOCK) + 1;

  // In order to simplify time comparison, we sacrify the MSB to avoid overflow
  // as the given amount of time must be short
  uint32_t time = (current_time & 0x7fffffff) + ticks;
  event->time = current_time + ticks;


  // Enqueue the event in the wait list.
  while (current && (current->time & 0x7fffffff) < time)
  {
    prev = current;
    current = current->next;
  }

  if (prev)
  {
    prev->next = event;
  }
  else
  {
    set_irq = 1;
    first_delayed = event;
  }
  event->next = current;

  // And finally update the timer trigger time in case we enqueued the event
  // at the head of the wait list.
  if (set_irq)
  {
    // This is important to reload the current time, in case the previous code
    // took too much time so that the interrupt is not missed
    uint32_t timer = hal_timer_count_get(hal_timer_fc_addr(0, 1)) + ticks;
    hal_timer_cmp_set(hal_timer_fc_addr(0, 1), timer);

    hal_timer_conf(
      hal_timer_fc_addr(0, 1), PLP_TIMER_ACTIVE, PLP_TIMER_RESET_DISABLED,
      PLP_TIMER_IRQ_ENABLED, PLP_TIMER_IEM_DISABLED, PLP_TIMER_CMPCLR_DISABLED,
      PLP_TIMER_ONE_SHOT_DISABLED, PLP_TIMER_REFCLK_ENABLED,
      PLP_TIMER_PRESCALER_DISABLED, 0, PLP_TIMER_MODE_64_DISABLED
    );
  }

  rt_irq_restore(irq);
}


void rt_time_wait_us(int time_us)
{
  rt_event_t *event = rt_event_get_blocking(NULL);
  rt_event_push_delayed(event, time_us);
  rt_event_wait(event);
}

RT_FC_BOOT_CODE void __attribute__((constructor)) __rt_time_init()
{
  int err = 0;

  __rt_time_conf_timer_default(); 

  rt_irq_set_handler(ARCHI_FC_EVT_TIMER1, __rt_timer_handler);
  rt_irq_mask_set(1<<ARCHI_FC_EVT_TIMER1);

  err |= __rt_cbsys_add(RT_CBSYS_POWEROFF, __rt_time_poweroff, NULL);
  err |= __rt_cbsys_add(RT_CBSYS_POWERON, __rt_time_poweron, NULL);

  if (err) rt_fatal("Unable to initialize time driver\n");
}

#if defined(ARCHI_HAS_FC)

#if defined(__LLVM__)
void __rt_timer_handler()
#else
void __attribute__((interrupt)) __rt_timer_handler()
#endif
{
  rt_event_t *event = first_delayed;

  uint32_t current_time = hal_timer_count_get(hal_timer_fc_addr(0, 1));

  // First dequeue and push to their scheduler all events with the same number of
  // ticks as they were waiting for the same time.
  while (event && (current_time - event->time) < 0x7fffffff)
  {
    rt_event_t *next = event->next;
    __rt_push_event(event->sched, event);
    event = next;
  }

  // Update the wait list with the next waiting event which has a different number
  // of ticks
  first_delayed = event;

  // Now re-arm the timer in case there are still some events
  if (first_delayed)
  {
    // Be carefull to set the new comparator from the current time plus a number of ticks
    // in order to set a value which is not before the actual count.
    // This may just delay a bit the events which is fine as the specified
    // duration is a minimum.
    hal_timer_cmp_set(hal_timer_fc_addr(0, 1),
      hal_timer_count_get(hal_timer_fc_addr(0, 1)) + 
      first_delayed->time - current_time
    );

    hal_timer_conf(
      hal_timer_fc_addr(0, 1), PLP_TIMER_ACTIVE, PLP_TIMER_RESET_DISABLED,
      PLP_TIMER_IRQ_ENABLED, PLP_TIMER_IEM_DISABLED, PLP_TIMER_CMPCLR_DISABLED,
      PLP_TIMER_ONE_SHOT_DISABLED, PLP_TIMER_REFCLK_ENABLED,
      PLP_TIMER_PRESCALER_DISABLED, 0, PLP_TIMER_MODE_64_DISABLED
    );
  }
  else
  {
    // Set back default state where timer is only counting with
    // no interrupt
    __rt_time_conf_timer_default(); 

    // Also clear timer interrupt as we might have a spurious one after
    // we entered the handler
    rt_irq_clr(1 << ARCHI_FC_EVT_TIMER1);
  }
}

#endif

#endif
