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
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#include "archi/pulp.h"

  .section .text
  .global _start
_start:

#include "rt/rt_data.h"
#include "archi/pulp.h"

#define LOAD_SYMBOL_2_GPR(gpr,symbol)  \
  .global symbol ;               \
  l.movhi gpr, hi(symbol) ;      \
  l.ori   gpr, gpr, lo(symbol)

#define LOAD_CONST_2_GPR(gpr,symbol)  \
    l.movhi gpr, hi(symbol) ;      \
    l.ori   gpr, gpr, lo(symbol)

  .section .vectors, "ax"
  .org 0x00
  l.j   reset_handler
  l.nop

  // This variable is pointed to the structure containing all information exchanged with
  // the platform loader. It is using a fixed address so that the loader can also find it
  // and then knows the address of the debug structure
  .org 0x90
  .global __rt_debug_struct_ptr
__rt_debug_struct_ptr:
  .word __hal_debug_struct




  .section .text

reset_handler:

  # clear the bss segment
  LOAD_SYMBOL_2_GPR(r5, _bss_start)
  LOAD_SYMBOL_2_GPR(r6, _bss_end)

  l.sfleu r6, r5
  l.bf    zero_loop_end
  l.nop   0
zero_loop:
  l.sw    -4(r6), r0
  l.addi  r6, r6, -4
  l.sfgtu r6, r5
  l.bf    zero_loop
  l.nop   0
zero_loop_end:


  /* Early Stack initilization */
  LOAD_SYMBOL_2_GPR(r1, stack)

  /* Do all other initializations from C code */
  l.jal    __rt_init
  l.nop

  /* Jump to main program entry point (argc = argv = 0) */
  l.addi   r3, r0, 0
  l.addi   r4, r0, 0
  l.jal    main
  l.nop


  /* If program returns from main, call exit routine */
  l.addi   r10, r11, 0
  l.jal    __rt_deinit
  l.nop

  l.addi   r3, r10, 0
  l.jal    exit
  l.nop
