#ifndef __PMSIS_TASK_H__
#define __PMSIS_TASK_H__

#include "pmsis/pmsis_types.h"

pi_task_t *pi_task_callback(pi_task_t *callback_task, void (*callback)(void*), void *arg);

pi_task_t *pi_task_callback_no_mutex(pi_task_t *callback_task, void (*func)(void *), void *arg);

static inline pi_task_t *pi_task_block(pi_task_t *callback_task);

pi_task_t *pi_task_block_no_mutex(pi_task_t *callback_task);

void pi_task_destroy(pi_task_t *task);

void pi_task_push(pi_task_t *task);

void pi_task_push_delayed_us(pi_task_t *task, uint32_t delay);

void pi_task_release(pi_task_t *task);

/**
 * Wait on the execution of the task associated to pi_task_t
 * Task must already have been initialized
 **/
void pi_task_wait_on(pi_task_t *task);

void pi_task_wait_on_no_mutex(pi_task_t *task);

#ifdef PMSIS_DRIVERS

#include "pmsis_hal/pmsis_hal.h"
#include "pmsis_backend/pmsis_backend_native_task_api.h"

static inline struct pi_task *pi_task(struct pi_task *task)
{
    pi_task_block(task);
    return task;
}

#else

#include "pmsis/implem/implem.h"

#endif  /* __PMSIS_TASK_H__ */

#endif
