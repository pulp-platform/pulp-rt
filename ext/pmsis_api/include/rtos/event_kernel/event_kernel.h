#ifndef __EVENT_KERNEL_H__
#define __EVENT_KERNEL_H__

#include "pmsis_types.h"

/** Typical event kernel main function **/
void pmsis_event_kernel_main(void *arg);

/** Allocate the structure and create a task if need be **/
int pmsis_event_kernel_init(struct pmsis_event_kernel_wrap **event_kernel,
        void (*event_kernel_entry)(void*));

/** Free the structure and kill the task if need be **/
void pmsis_event_kernel_destroy(struct pmsis_event_kernel_wrap **event_kernel);

/**
 * Allocate a set number of events in the scheduler
 **/
int pmsis_event_alloc(struct pmsis_event_kernel_wrap *event_kernel, int nb_events);

/**
  * Free up to the specified number of events
  * Might be less if nb_events is greater than the actual allocated nb of events
  * Freeing might also be delayed if some events are in use
  **/
int pmsis_event_free(struct pmsis_event_kernel_wrap *wrap, int nb_events);

/**
 * Affect task to an allocated event
 * May be called from either cluster or fc
 **/
int pmsis_event_push(struct pmsis_event_kernel_wrap *event_kernel, pi_fc_task_t *task);

/**
 * Wait on the execution of the task associated to pi_fc_task_t
 * Task must already have been initialized
 **/
void mc_wait_on_task(pi_fc_task_t *task);

void pmsis_event_kernel_mutex_release(struct pmsis_event_kernel_wrap *wrap);

void pmsis_event_lock_cl_to_fc_init(struct pmsis_event_kernel_wrap *wrap);

#endif
