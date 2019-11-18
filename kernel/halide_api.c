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

/*
 * Authors: Samuel Riedel, ETH (sriedel@iis.ee.ethz.ch)
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "rt/rt_api.h"

void *halide_malloc(void *user_context, size_t x) {
  return rt_alloc(RT_ALLOC_CL_DATA, x);
}

void halide_free(void *user_context, void *ptr) {
  // FIXME: Modify allocator to deal with size correctly
  rt_free(RT_ALLOC_CL_DATA, ptr, 4);
}

char *getenv(const char *name) { return NULL; };

size_t write(int fd, const void *buf, size_t count) {
  const char *msg = buf;
  printf("Write is not implemented! fd: %d\n", fd);
  for (unsigned i = 0; i < count; ++i) {
    printf("%c", msg[count]);
  }
  printf("\n");
  return 1;
}

FILE *fopen(const char *pathname, const char *mode) {
  printf("fopen not implemented!\n");
  return (FILE *)NULL;
}

int fclose(FILE *stream) {
  printf("fclose not implemented!\n");
  return -1;
}

int fileno(FILE *stream) {
  printf("fileno not implemented!\n");
  return -1;
}

void halide_print(void *user_context, const char *msg) { printf("%s\n", msg); }

/////////////
// Atomics //
/////////////

///////////
// LR/SC //
///////////
#define LR(bytes, type)                                                        \
  inline type __load_reserved_##bytes(type *mem, int memorder) {               \
    type ret;                                                                  \
    __asm__ volatile("lr.w %[ret], (%[mem])"                                   \
                     : [ ret ] "=r"(ret)                                       \
                     : [ mem ] "r"(mem), "m"(mem));                            \
    return ret;                                                                \
  }

LR(1, uint8_t)
LR(2, uint16_t)
LR(4, uint32_t)
LR(8, uint64_t)

#define SC(bytes, type)                                                        \
  inline type __store_conditional_##bytes(type *mem, type val, int memorder) { \
    type ret;                                                                  \
    __asm__ volatile("sc.w %[ret], %[val], (%[mem])"                           \
                     : [ ret ] "=r"(ret)                                       \
                     : [ val ] "r"(val), [ mem ] "r"(mem), "m"(mem));          \
    return ret;                                                                \
  }

SC(1, uint8_t)
SC(2, uint16_t)
SC(4, uint32_t)
SC(8, uint64_t)

/////////
// GCC //
/////////
// #define MEMORY_ORDER_RELAXED 0
// #define MEMORY_ORDER_CONSUME 1
// #define MEMORY_ORDER_ACQUIRE 2
// #define MEMORY_ORDER_RELEASE 3
// #define MEMORY_ORDER_ACQ_REL 4
// #define MEMORY_ORDER_SEQ_CST 5

/////////////////
// Atomic Load //
/////////////////
// void __atomic_load (size_t size, void *ptr, void *return, int memorder)

#define ATOMIC_LOAD(bytes, type)                                               \
  inline type __atomic_load_##bytes(type *ptr, int memorder) {                 \
    type ret;                                                                  \
    __asm__ volatile("lr.w %[ret], (%[ptr])"                                   \
                     : [ ret ] "=r"(ret)                                       \
                     : [ ptr ] "r"(ptr), "m"(ptr));                            \
    return ret;                                                                \
  }

ATOMIC_LOAD(1, uint8_t)
ATOMIC_LOAD(2, uint16_t)
ATOMIC_LOAD(4, uint32_t)
ATOMIC_LOAD(8, uint64_t)

//////////////////
// Atomic Store //
//////////////////
// void __atomic_store (size_t size, void *mem, void *val, int memorder)

#define ATOMIC_STORE(bytes, type)                                              \
  inline void __atomic_store_##bytes(type *ptr, type val, int memorder) {      \
    register int reg_zero asm("x0");                                           \
    __asm__ volatile("amoswap.w %[ret], %[val], (%[ptr])"                      \
                     : [ ret ] "=r"(reg_zero)                                  \
                     : [ val ] "r"(val), [ ptr ] "r"(ptr), "m"(ptr));          \
  }

ATOMIC_STORE(1, uint8_t)
ATOMIC_STORE(2, uint16_t)
ATOMIC_STORE(4, uint32_t)
ATOMIC_STORE(8, uint64_t)

/////////////////////
// Atomic Exchange //
/////////////////////
// void __atomic_exchange (type *ptr, type *val, type *ret, int memorder)
#define ATOMIC_EXCHANGE(bytes, type)                                           \
  inline type __atomic_exchange_##bytes(type *ptr, type val, int memorder) {   \
    type ret;                                                                  \
    __asm__ __volatile__("" : : : "memory");                                   \
    __asm__ volatile("amoswap.w %[ret], %[val], (%[ptr])"                      \
                     : [ ret ] "=r"(ret), "+m"(*ptr)                           \
                     : [ val ] "r"(val), [ ptr ] "r"(ptr));                    \
    __asm__ __volatile__("" : : : "memory");                                   \
    return ret;                                                                \
  }

ATOMIC_EXCHANGE(1, uint8_t)
ATOMIC_EXCHANGE(2, uint16_t)
ATOMIC_EXCHANGE(4, uint32_t)
ATOMIC_EXCHANGE(8, uint64_t)

////////////////
// Atomic CAS //
////////////////
// bool __atomic_compare_exchange (size_t size, void *obj, void *expected, void
// *desired, int success_memorder, int failure_memorder)
#define ATOMIC_CAS(bytes, type)                                                \
  inline bool __atomic_compare_exchange_##bytes(                               \
      type *ptr, type *expected, type desired, bool weak,                      \
      int success_memorder, int failure_memorder) {                            \
    type prev = *expected;                                                     \
    while (1) {                                                                \
      *expected = __load_reserved_##bytes(ptr, success_memorder);              \
      if (prev != *expected) {                                                 \
        return false;                                                          \
      }                                                                        \
      if (__store_conditional_##bytes(ptr, desired, success_memorder) == 0) {  \
        return true;                                                           \
      }                                                                        \
    }                                                                          \
  }

ATOMIC_CAS(1, uint8_t)
ATOMIC_CAS(2, uint16_t)
ATOMIC_CAS(4, uint32_t)
ATOMIC_CAS(8, uint64_t)

//////////////////////
// Atomic Operation //
//////////////////////
// type __atomic_fetch_add (type *ptr, type val, int memorder)
#define ATOMIC_OP(bytes, type, op)                                             \
  inline type __atomic_fetch_##op##_##bytes(type *mem, type val,               \
                                            int memorder) {                    \
    type ret;                                                                  \
    __asm__ __volatile__("" : : : "memory");                                   \
    __asm__ volatile("amo" #op ".w %[ret], %[val], (%[mem])"                   \
                     : [ ret ] "=r"(ret), "+m"(*mem)                           \
                     : [ val ] "r"(val), [ mem ] "r"(mem));                    \
    __asm__ __volatile__("" : : : "memory");                                   \
    return ret;                                                                \
  }

#define ATOMIC_SUB(bytes, type)                                                \
  inline type __atomic_fetch_sub_##bytes(type *mem, type val, int memorder) {  \
    type ret;                                                                  \
    __asm__ __volatile__("" : : : "memory");                                   \
    __asm__ volatile("amoadd.w %[ret], %[val], (%[mem])"                       \
                     : [ ret ] "=r"(ret), "+m"(*mem)                           \
                     : [ val ] "r"(-val), [ mem ] "r"(mem));                   \
    __asm__ __volatile__("" : : : "memory");                                   \
    return ret;                                                                \
  }

ATOMIC_OP(1, uint8_t, add)
ATOMIC_OP(2, uint16_t, add)
ATOMIC_OP(4, uint32_t, add)
ATOMIC_OP(8, uint64_t, add)

ATOMIC_OP(1, uint8_t, and)
ATOMIC_OP(2, uint16_t, and)
ATOMIC_OP(4, uint32_t, and)
ATOMIC_OP(8, uint64_t, and)

ATOMIC_OP(1, uint8_t, or)
ATOMIC_OP(2, uint16_t, or)
ATOMIC_OP(4, uint32_t, or)
ATOMIC_OP(8, uint64_t, or)

ATOMIC_OP(1, uint8_t, xor)
ATOMIC_OP(2, uint16_t, xor)
ATOMIC_OP(4, uint32_t, xor)
ATOMIC_OP(8, uint64_t, xor)

ATOMIC_SUB(1, int8_t)
ATOMIC_SUB(2, int16_t)
ATOMIC_SUB(4, int32_t)
ATOMIC_SUB(8, int64_t)
