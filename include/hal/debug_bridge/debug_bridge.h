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

typedef enum {
  HAL_BRIDGE_REQ_CONNECT = 0,
  HAL_BRIDGE_REQ_DISCONNECT = 1,
  HAL_BRIDGE_REQ_OPEN = 2,
  HAL_BRIDGE_REQ_READ = 3,
  HAL_BRIDGE_REQ_WRITE = 4,
  HAL_BRIDGE_REQ_CLOSE = 5,
  HAL_BRIDGE_REQ_FIRST_USER = 5
} hal_bridge_req_e;

typedef struct hal_bridge_req_s {
  uint32_t next;
  uint32_t size;
  uint32_t type;
  uint32_t done;
  uint32_t popped;
  union {
    struct {
      uint32_t name_len;
      uint32_t name;
      uint32_t flags;
      uint32_t mode;
      uint32_t retval;
    } open;
    struct {
      uint32_t file;
      uint32_t retval;
    } close;
    struct {
      uint32_t file;
      uint32_t ptr;
      uint32_t len;
      uint32_t retval;
    } read;
    struct {
      uint32_t file;
      uint32_t ptr;
      uint32_t len;
      uint32_t retval;
    } write;
  };
} hal_bridge_req_t;

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

  // Requests
  uint32_t first_req;
  uint32_t last_req;
  uint32_t first_bridge_req;

  uint32_t notif_req_addr;
  uint32_t notif_req_value;

  uint32_t bridge_connected;

} hal_debug_struct_t;

typedef hal_debug_struct_t hal_bridge_t;

extern hal_debug_struct_t HAL_DEBUG_STRUCT_NAME;

static inline hal_debug_struct_t *hal_debug_struct_get()
{
  return &HAL_DEBUG_STRUCT_NAME;
}


static inline hal_bridge_t *hal_bridge_get()
{
  return hal_debug_struct_get();
}


#define HAL_DEBUG_STRUCT_INIT {0, 1, 0 ,0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0}


static inline int hal_bridge_is_connected(hal_bridge_t *bridge) {
  return *(volatile uint32_t *)&bridge->bridge_connected;
}

static inline void hal_bridge_connect(hal_bridge_req_t *req)
{
  req->type = HAL_BRIDGE_REQ_CONNECT;
}

static inline void hal_bridge_disconnect(hal_bridge_req_t *req)
{
  req->type = HAL_BRIDGE_REQ_DISCONNECT;
}

static inline void hal_bridge_open(hal_bridge_req_t *req, int name_len, const char* name, int flags, int mode)
{
  req->type = HAL_BRIDGE_REQ_OPEN;
  req->open.name_len = name_len;
  req->open.name = (uint32_t)(long)name;
  req->open.flags = flags;
  req->open.mode = mode;
}

static inline void hal_bridge_close(hal_bridge_req_t *req, int file)
{
  req->type = HAL_BRIDGE_REQ_CLOSE;
  req->close.file = file;
}

static inline void hal_bridge_read(hal_bridge_req_t *req, int file, void* ptr, int len)
{
  req->type = HAL_BRIDGE_REQ_READ;
  req->read.file = file;
  req->read.ptr = (uint32_t)(long)ptr;
  req->read.len = len;
}

static inline void hal_bridge_write(hal_bridge_req_t *req, int file, void* ptr, int len)
{
  req->type = HAL_BRIDGE_REQ_WRITE;
  req->write.file = file;
  req->write.ptr = (uint32_t)(long)ptr;
  req->write.len = len;
}


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
