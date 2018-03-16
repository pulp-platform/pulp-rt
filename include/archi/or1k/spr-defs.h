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

#ifndef _ARCHI_OR1K_SPR_DEFS_H
#define _ARCHI_OR1K_SPR_DEFS_H

/* Definition of special-purpose registers (SPRs). */
#define MAX_GRPS (32)
#define MAX_SPRS_PER_GRP_BITS (11)
#define MAX_SPRS_PER_GRP (1 << MAX_SPRS_PER_GRP_BITS)
#define MAX_SPRS (0x10000)

/* Base addresses for the groups */
#define SPRGROUP_SYS	 (0<< MAX_SPRS_PER_GRP_BITS)

/* System control and status group */
#define SPR_EPCR_BASE  (SPRGROUP_SYS + 32)

/* PULP registers */
#define SPR_CORE_ID    (SPRGROUP_SYS + 0x680)
#define SPR_CLUSTER_ID (SPRGROUP_SYS + 0x681)

#endif	/* SPR_DEFS__H */
