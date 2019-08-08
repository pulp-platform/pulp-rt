#ifndef __PMSIS_OS_H__
#define __PMSIS_OS_H__

#include <stdint.h>
#include <stddef.h>
#include "pmsis.h"

#include "pmsis/rtos/malloc/pmsis_malloc.h"
#include "pmsis/rtos/malloc/pmsis_l1_malloc.h"
#include "pmsis/rtos/malloc/pmsis_l2_malloc.h"
#if (defined(__GAP8__) && defined(__USE_TCDM_MALLOC__))
    #include "rtos/malloc/pmsis_fc_tcdm_malloc.h"
#endif
#include "pmsis/rtos/os_frontend_api/pmsis_task.h"
#include "pmsis/rtos/event_kernel/event_kernel.h"

#include "pmsis_backend/pmsis_backend_native_types.h"
#include "pmsis_backend/pmsis_backend_native_task_api.h"

/** Kickoff the system : Must be called in the main
 * Completely OS dependant might do anything from a function call to main task 
 * creation */
static inline int pmsis_kickoff(void *arg);

#ifdef PMSIS_DRIVERS

/** Kickoff the system : Must be called in the main
 * Completely OS dependant might do anything from a function call to main task 
 * creation */
static inline int pmsis_kickoff(void *arg)
{
    return __os_native_kickoff(arg);
}

#endif

#endif  /* __PMSIS_OS_H__ */
