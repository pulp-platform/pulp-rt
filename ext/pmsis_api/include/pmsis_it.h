#ifndef __PMSIS_IT__
#define __PMSIS_IT__

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Machine mode IRQ handler wrapper */
#define HANDLER_WRAPPER(f)                                                     \
__attribute__((section(".text")))                                              \
static inline void __handler_wrapper_##f (void)                                \
{                                                                              \
    asm volatile( "addi sp, sp, -18*4\n\t"                                     \
                  "sw   ra, 15*4(sp)\n\t"                                      \
                  "jal  Exception_handler\n\t");                               \
    asm volatile("jalr  %0" :: "r" (f));                                       \
    asm volatile("j    SVC_Context");                                          \
}

/* Machine mode IRQ handler wrapper light version */
#define HANDLER_WRAPPER_LIGHT(f)                                    \
__attribute__((section(".text")))                                   \
static inline void __handler_wrapper_light_##f (void)               \
{                                                                   \
    asm volatile( "addi sp, sp, -11*4\n\t"                          \
                  "sw    ra,  32(sp)\n\t"                           \
                  "sw    a0,  28(sp)\n\t"                           \
                  "sw    a1,  24(sp)\n\t"                           \
                  "sw    a2,  20(sp)\n\t"                           \
                  "sw    a3,  16(sp)\n\t"                           \
                  "sw    a4,  12(sp)\n\t"                           \
                  "sw    a5,  8(sp)\n\t"                            \
                  "sw    a6,  4(sp)\n\t"                            \
                  "sw    a7,  0(sp)\n\t");                          \
    asm volatile("jalr  %0" :: "r" (f));                            \
    asm volatile("lw     ra,  32(sp)\n\t"                           \
                 "lw     a0,  28(sp)\n\t"                           \
                 "lw     a1,  24(sp)\n\t"                           \
                 "lw     a2,  20(sp)\n\t"                           \
                 "lw     a3,  16(sp)\n\t"                           \
                 "lw     a4,  12(sp)\n\t"                           \
                 "lw     a5,  8(sp)\n\t"                            \
                 "lw     a6,  4(sp)\n\t"                            \
                 "lw     a7,  0(sp)\n\t"                            \
                 "addi   sp, sp, 11*4\n\t"                          \
                 "mret\n\t");                                       \
}
#endif
