#ifndef __PMSIS_TASK_H__
#define __PMSIS_TASK_H__

// used to avoid OS dependency or in bare metal mode
//#define __NO_NATIVE_MUTEX__

#include "pmsis_types.h"
#include "pmsis_backend_native_task_api.h"

// define task priorities
#define PMSIS_TASK_MAX_PRIORITY 2
#define PMSIS_TASK_OS_PRIORITY 1
#define PMSIS_TASK_USER_PRIORITY 0

static inline int disable_irq(void);

static inline void restore_irq(int irq_enable);

static inline void pmsis_mutex_take(pmsis_mutex_t *mutex);

static inline void pmsis_mutex_release(pmsis_mutex_t *mutex);

static inline int pmsis_mutex_init(pmsis_mutex_t *mutex);

static inline int pmsis_mutex_deinit(pmsis_mutex_t *mutex);

static inline void pmsis_spinlock_init(pmsis_spinlock_t *spinlock);

static inline void pmsis_spinlock_take(pmsis_spinlock_t *spinlock);

static inline void pmsis_spinlock_release(pmsis_spinlock_t *spinlock);

static inline void *pmsis_task_create(void (*entry)(void*),
        void *arg,
        char *name,
        int priority);

static inline void pmsis_task_suspend(__os_native_task_t *task);

pi_fc_task_t *mc_task_callback(pi_fc_task_t *callback_task, void (*callback)(void*), void *arg);

static inline struct pi_fc_task *mc_task(struct pi_fc_task *task)
{
  task->id = FC_TASK_NONE_ID;
  task->arg[0] = (uint32_t)0;
  return task;
}

static inline void pmsis_exit(int err);

void pi_yield();

#ifndef PMSIS_NO_INLINE_INCLUDE

#include "pmsis_hal.h"

/*
 * Disable IRQs while saving previous irq state
 */
static inline int disable_irq(void)
{
    hal_compiler_barrier();
    return __os_native_api_disable_irq();
}

/*
 * restore IRQs state
 */
static inline void restore_irq(int irq_enable)
{
    hal_compiler_barrier();
    __os_native_api_restore_irq(irq_enable);
}

/*
 * lock a mutex and begin critical section
 */
static inline void pmsis_mutex_take(pmsis_mutex_t *mutex)
{
    hal_compiler_barrier();
#ifdef __NO_NATIVE_MUTEX__
    int irq_enabled;
    volatile int mutex_free=0;
    while(!mutex_free)
    {
        irq_enabled = disable_irq();
        hal_compiler_barrier();
        mutex_free = !((volatile uint32_t)mutex->mutex_object);
        hal_compiler_barrier();
        restore_irq(irq_enabled);
    }
    irq_enabled = disable_irq();
    mutex->mutex_object = (void*)1;
    restore_irq(irq_enabled);
#else
    mutex->take(mutex->mutex_object);
#endif
}

/*
 * unlock mutex and end critical section
 */
static inline void pmsis_mutex_release(pmsis_mutex_t *mutex)
{
    hal_compiler_barrier();
#ifdef __NO_NATIVE_MUTEX__
    int irq_enabled = disable_irq();
    hal_compiler_barrier();
    mutex->mutex_object = (void*)0;
    hal_compiler_barrier();
    restore_irq(irq_enabled);
#else
    mutex->release(mutex->mutex_object);
#endif
}

/*
 * init a mutex, return non zero in case of failure
 */
static inline int pmsis_mutex_init(pmsis_mutex_t *mutex)
{
    hal_compiler_barrier();
#ifdef __NO_NATIVE_MUTEX__
    mutex->mutex_object = (void*)0;
    return 0;
#else
    return  __os_native_api_mutex_init(mutex);
#endif
}

/*
 * deinit a mutex, return non zero in case of failure
 */
static inline int pmsis_mutex_deinit(pmsis_mutex_t *mutex)
{
    hal_compiler_barrier();
#ifdef __NO_NATIVE_MUTEX__
    mutex->mutex_object = (void*)0;
    return 0;
#else
    return  __os_native_api_mutex_deinit(mutex);
#endif
}

static inline void pmsis_spinlock_init(pmsis_spinlock_t *spinlock)
{
    hal_compiler_barrier();
    spinlock->lock = 0;
}

static inline void pmsis_spinlock_take(pmsis_spinlock_t *spinlock)
{
    int irq_enabled = disable_irq();
    hal_compiler_barrier();
    spinlock->lock = 1;
    hal_compiler_barrier();
    restore_irq(irq_enabled);
}

static inline void pmsis_spinlock_release(pmsis_spinlock_t *spinlock)
{
    int irq_enabled = disable_irq();
    hal_compiler_barrier();
    spinlock->lock = 0;
    hal_compiler_barrier();
    restore_irq(irq_enabled);
}

static inline void *pmsis_task_create(void (*entry)(void*),
        void *arg,
        char *name,
        int priority)
{
    return __os_native_api_create_task(entry, arg, name, priority);
}

static inline void pmsis_task_suspend(__os_native_task_t *task)
{
    __os_native_task_suspend(task);
}

static inline void pmsis_exit(int err)
{
    exit(err);
}

#endif  /* __PMSIS_TASK_H__ */

#endif