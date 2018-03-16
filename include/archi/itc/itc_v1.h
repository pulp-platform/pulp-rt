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

#ifndef __ARCHI_ITC_ITC_V1_H__
#define __ARCHI_ITC_ITC_V1_H__

#define ARCHI_ITC_MASK        0x00
#define ARCHI_ITC_MASK_SET    0x04
#define ARCHI_ITC_MASK_CLR    0x08

#define ARCHI_ITC_STATUS      0x0C
#define ARCHI_ITC_STATUS_SET  0x10
#define ARCHI_ITC_STATUS_CLR  0x14

#define ARCHI_ITC_ACK         0x18
#define ARCHI_ITC_ACK_SET     0x1C
#define ARCHI_ITC_ACK_CLR     0x20

#define ARCHI_ITC_FIFO        0x24

#endif