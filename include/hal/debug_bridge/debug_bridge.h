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

#ifndef __HAL_DEBUG_BRIDGE_DEBUG_BRIDGE_H__
#define __HAL_DEBUG_BRIDGE_DEBUG_BRIDGE_H__

#include <stdint.h>

#define HAL_DEBUG_STRUCT_NAME __hal_debug_struct
#define HAL_DEBUG_STRUCT_NAME_STR "__hal_debug_struct"

#define HAL_PRINTF_BUF_SIZE 128

// This structure can be used to interact with the host loader
typedef struct {

	// Used by external debug bridge to get exit status when using the board
	uint32_t exit_status;

	// Printf
  uint32_t use_internal_printf;
  uint32_t pending_putchar;
  uint32_t putc_current;
  uint8_t putc_buffer[HAL_PRINTF_BUF_SIZE];

  // Debug step, used for showing progress to host loader
	uint32_t debug_step;
	uint32_t debug_step_pending;

} hal_debug_struct_t;

extern hal_debug_struct_t HAL_DEBUG_STRUCT_NAME;

static inline hal_debug_struct_t *hal_debug_struct_get()
{
  return &HAL_DEBUG_STRUCT_NAME;
}



#define HAL_DEBUG_STRUCT_INIT {0, 1, 0 ,0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0}



static inline void hal_debug_flush_printf(hal_debug_struct_t *debug_struct) {
  while(*(volatile uint32_t *)&debug_struct->pending_putchar);
}

static inline void hal_debug_exit(hal_debug_struct_t *debug_struct, int status) {
  *(volatile uint32_t *)&debug_struct->exit_status = 0x80000000 | status;
}

static inline void hal_debug_putchar(hal_debug_struct_t *debug_struct, char c) {
  hal_debug_flush_printf(debug_struct);
  *(volatile uint8_t *)&(debug_struct->putc_buffer[debug_struct->putc_current++]) = c;
  if (*(volatile uint32_t *)&debug_struct->putc_current == HAL_PRINTF_BUF_SIZE || c == '\n') {
    *(volatile uint32_t *)&debug_struct->pending_putchar = debug_struct->putc_current;
    *(volatile uint32_t *)&debug_struct->putc_current = 0;
  }
}

static inline void hal_debug_step(hal_debug_struct_t *debug_struct, unsigned int value) {
  *(volatile uint32_t *)&debug_struct->debug_step = value;
  *(volatile uint32_t *)&debug_struct->debug_step_pending = 1;
  while (*(volatile uint32_t *)&debug_struct->debug_step_pending);
}

static inline void hal_debug_reset(hal_debug_struct_t *debug_struct) {
  *(volatile uint32_t *)&debug_struct->exit_status = 0x80000000 | 0x40000000;    // 0xC0000000: reset
}

#endif
