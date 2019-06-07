#ifndef __PMSIS_OS_H__

#include <stdint.h>
#include <stddef.h>
#include "pmsis.h"

#include "rtos/malloc/pmsis_malloc.h"
#include "rtos/malloc/pmsis_l1_malloc.h"
#include "rtos/malloc/pmsis_l2_malloc.h"
#include "rtos/malloc/pmsis_hyperram_malloc.h"
#ifdef __GAP8__
    #include "pmsis_fc_tcdm_malloc.h"
#endif
#include "rtos/os_frontend_api/pmsis_task.h"
#include "rtos/event_kernel/event_kernel.h"

#include "pmsis_backend_native_types.h"
#include "pmsis_backend_native_task_api.h"

/** Kickoff the system : Must be called in the main
 * Completely OS dependant might do anything from a function call to main task 
 * creation */
static inline int pmsis_kickoff(void *arg);

static inline uint32_t pi_core_id();

static inline uint32_t pi_cluster_id();

static inline uint32_t pi_is_fc();

static inline uint32_t pi_nb_custer_cores();


#ifndef PMSIS_NO_INLINE_INCLUDE

/** Kickoff the system : Must be called in the main
 * Completely OS dependant might do anything from a function call to main task 
 * creation */
static inline int pmsis_kickoff(void *arg)
{
    return __os_native_kickoff(arg);
}

static inline uint32_t pi_core_id()
{
  return __native_core_id();
}

static inline uint32_t pi_cluster_id()
{
  return __native_cluster_id();
}

static inline uint32_t pi_is_fc()
{
  return __native_is_fc();
}

static inline uint32_t pi_nb_cluster_cores()
{
  return PI_CLUSTER_NB_CORES;
}

#endif

#endif
