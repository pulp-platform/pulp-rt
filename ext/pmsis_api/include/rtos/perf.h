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

#ifndef __RTOS_OS_FRONTEND_API_PERF_H__
#define __RTOS_OS_FRONTEND_API_PERF_H__

/**        
 * @ingroup groupCluster       
 */

/**        
 * @defgroup Perf Performance counters
 *
 * This API gives access to the core performance counters. Each core has a few performance counters which can be configured 
 * to count one event out of several available. An event is a cycle, an instruction, a cache miss and so on. The number of counters
 * limits the number of events which can be monitored at the same time and depends on the architecture and the platform.
 *
 * In addition, this API uses a few other HW mechanisms useful for monitoring performance such as timers.
 *
 * To use the API, a structure of type pi_perf_t must be allocated and passed to most of the calls. This structure
 * contains the desired configuration and is used to save the values of the performance counters.
 * It can be used by one core or several (if concurrent accesses are protected).
 * The idea is that the hardware counters can be reset, started and stopped in order to get the event values for
 * a specific period and this can then be cumulated to the performance structure.
 */

/**        
 * @addtogroup Perf
 * @{        
 */

/** \enum pi_perf_event_e
 * \brief Performance event identifiers.
 *
 * This can be used to describe which performance event to monitor (cycles, cache miss, etc).
 */
typedef enum {
  PI_PERF_CYCLES        = CSR_PCER_NB_EVENTS,    /*!< Total number of cycles (also includes the cycles where the core is sleeping). Be careful that this event is using a timer shared within the cluster, so resetting, starting or stopping it on one core will impact other cores of the same cluster. */
  PI_PERF_ACTIVE_CYCLES = CSR_PCER_CYCLES,       /*!< Counts the number of cycles the core was active (not sleeping). */
  PI_PERF_INSTR         = CSR_PCER_INSTR,        /*!< Counts the number of instructions executed. */
  PI_PERF_LD_STALL      = CSR_PCER_LD_STALL,     /*!< Number of load data hazards. */  
  PI_PERF_JR_STALL      = CSR_PCER_JMP_STALL,    /*!< Number of jump register data hazards. */
  PI_PERF_IMISS         = CSR_PCER_IMISS,        /*!< Cycles waiting for instruction fetches, i.e. number of instructions wasted due to non-ideal caching. */
  PI_PERF_LD            = CSR_PCER_LD,           /*!< Number of data memory loads executed. Misaligned accesses are counted twice. */
  PI_PERF_ST            = CSR_PCER_ST,           /*!< Number of data memory stores executed. Misaligned accesses are counted twice. */
  PI_PERF_JUMP          = CSR_PCER_JUMP,         /*!< Number of unconditional jumps (j, jal, jr, jalr). */
  PI_PERF_BRANCH        = CSR_PCER_BRANCH,       /*!< Number of branches. Counts both taken and not taken branches. */
  PI_PERF_BTAKEN        = CSR_PCER_TAKEN_BRANCH, /*!< Number of taken branches. */
  PI_PERF_RVC           = CSR_PCER_RVC,          /*!< Number of compressed instructions executed. */
#ifdef CSR_PCER_ELW
  PI_PERF_ELW           = CSR_PCER_ELW,          /*!< Number of cycles wasted due to ELW instruction. */
#endif
  PI_PERF_LD_EXT        = CSR_PCER_LD_EXT,       /*!< Number of memory loads to EXT executed. Misaligned accesses are counted twice. Every non-TCDM access is considered external (cluster only). */
  PI_PERF_ST_EXT        = CSR_PCER_ST_EXT,       /*!< Number of memory stores to EXT executed. Misaligned accesses are counted twice. Every non-TCDM access is considered external (cluster only). */
  PI_PERF_LD_EXT_CYC    = CSR_PCER_LD_EXT_CYC,   /*!< Cycles used for memory loads to EXT. Every non-TCDM access is considered external (cluster only). */
  PI_PERF_ST_EXT_CYC    = CSR_PCER_ST_EXT_CYC,   /*!< Cycles used for memory stores to EXT. Every non-TCDM access is considered external (cluster only). */
  PI_PERF_TCDM_CONT     = CSR_PCER_TCDM_CONT,    /*!< Cycles wasted due to TCDM/log-interconnect contention (cluster only). */
} pi_perf_event_e;



/** \brief Configure performance counters.
 *
 * The set of events which can be activated at the same time depends on the architecture and the platform. On real chips (rather than with
 * the simulator), there is always only one counter. It is advisable to always use only one to be compatible with simulator and chip.
 *
 * At least PI_PERF_CYCLES and another event can be monitored at the same time as the first one is using the timer.
 *
 * \param perf  A pointer to the performance structure.
 * \param events A mask containing the events to activate. This is a bitfield, so events identifier must be used like 1 << PI_PERF_CYCLES.
 */
static inline void pi_perf_conf(unsigned events);



/** \brief Reset all hardware performance counters.
 *
 * All hardware performance counters are set to 0.
 * Note that this does not modify the value of the counters in the specified structure,
 * this must be done by calling pi_perf_init.
 *
 * \param perf  A pointer to the performance structure.
 */
static inline void pi_perf_reset();



/** \brief Start monitoring configured events.
 *
 * This function is useful for finely controlling the point where performance events start being monitored.
 * The counter retains its value between stop and start so it is possible to easily sum events for several
 * portions of code.
 *
 * \param perf  A pointer to the performance structure.
 */
static inline void pi_perf_start();



/** \brief Stop monitoring configured events.
 *
 * \param perf  A pointer to the performance structure.
 */
static inline void pi_perf_stop();




/** \brief Read a performance counter
 *
 * This does a direct read to the specified performance counter. Calling this functions is useful for getting
 * the performance counter with very low overhead (just few instructions).
 * 
 * \param id    The performance event identifier to read.
 * \return      The performance counter value.
 */
static inline unsigned int pi_perf_read(int id);

//!@}

#endif