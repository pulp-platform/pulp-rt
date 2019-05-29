#ifndef __PMSIS_INTRSC__
#define __PMSIS_INTRSC__

static inline uint32_t __native_asm_bit_set_count(uint32_t reg)
{
    uint32_t cnt;
    asm volatile ("p.cnt %0, %1 \n\t"
                  : "=r" (cnt)  : "r"(reg));
    return cnt;
}


static inline uint32_t __native_asm_bit_clr_count(uint32_t reg)
{
    uint32_t cnt;
    asm volatile ("p.clb %0, %1 \n\t"
                  : "=r" (cnt)  : "r"(reg));
    return cnt;
}
#endif
