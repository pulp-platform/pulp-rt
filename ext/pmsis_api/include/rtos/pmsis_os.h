#ifndef __PMSIS_OS_H__

#include <stdint.h>
#include <stddef.h>
#include "pmsis.h"

#include "pmsis_malloc.h"
#include "pmsis_l1_malloc.h"
#include "pmsis_l2_malloc.h"
#ifdef __GAP8__
    #include "pmsis_fc_tcdm_malloc.h"
#endif
#include "pmsis_task.h"
#include "event_kernel.h"
#include "core_utils.h"

#include "pmsis_backend_native_types.h"
#include "pmsis_backend_native_task_api.h"

/** Kickoff the system : Must be called in the main
 * Completely OS dependant might do anything from a function call to main task 
 * creation */
static inline int pmsis_kickoff(void *arg)
{
    return __os_native_kickoff(arg);
}

#endif
