/*
 * Copyright (C) 2018 ETH Zurich, University of Bologna
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


#ifndef __ARCHI_CHIPS_FULMINE_PROPERTIES_H__
#define __ARCHI_CHIPS_FULMINE_PROPERTIES_H__


/*
 * FEATURES
 */ 



/*
 * MEMORIES
 */ 

#define ARCHI_HAS_L2                   1
#define ARCHI_HAS_L1                   1



/*
 * IP VERSIONS
 */

#define MCHAN_VERSION       5
#define EU_VERSION          1
#define PERIPH_VERSION      1
#define ICACHE_CTRL_VERSION 1
#define TIMER_VERSION       1
#define APB_SOC_VERSION     1
#define STDOUT_VERSION      2
#define OR1K_VERSION        5


/*
 * CLUSTER
 */

#define ARCHI_HAS_CLUSTER   1
#define ARCHI_L1_TAS_BIT    20





/*
 * CLOCKS
 */

#define ARCHI_REF_CLOCK_LOG2 15
#define ARCHI_REF_CLOCK      (1<<ARCHI_REF_CLOCK_LOG2)




/*
 * CLUSTER EVENTS
 */


#define ARCHI_EVT_DMA    8



#endif
