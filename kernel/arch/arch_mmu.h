/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _ARCH_MMU_H_
#define _ARCH_MMU_H_

#include <sys/types.h>

// Invalidates all TLB entries
#define arch_tlb_invalidate_all()\
({\
    asm volatile ("tlbi vmalle1is\n"\
                  "dsb sy\n"\
                  "isb sy\n");\
})

// Invalidate VA for only entries with the specified ASID
#define arch_tlb_invalidate_va(va, asid)\
({\
    asm volatile ("lsr %0, %0, #12\n"\
                  "bfi %0, xzr, #48, #16\n"\
                  "bfi %0, %1, #48, #16\n"\
                  "tlbi vale1is, %0\n"\
                  "dsb sy\n"\
                  "isb sy\n"\
                  : "+r" (va)\
                  : "r" (asid));\
})

// Invalidate all entries corresponding to the specified ASID
#define arch_tlb_invalidate_asid(asid)\
({\
    asm volatile ("mov x1, xzr\n"\
                  "bfi x1, %0, #48, #16\n"\
                  "tlbi aside1is, x1\n"\
                  "dsb sy\n"\
                  "isb sy\n"\
                  :: "r" (asid)\
                  : "x1");\
})

// MMU feature support checkers
#define arch_mmu_is_4kb_granule_supported()\
({\
    unsigned long result = false;\
    asm ("mrs %0, ID_AA64MMFR0_EL1\n"\
         "ubfx %0, %0, #28, #4\n"\
         "cmp %0, #0\n"\
         "cset %0, eq\n"\
         : "+r" (result) :);\
    (bool)result;\
})
#define arch_mmu_is_16kb_granule_supported()\
({\
    unsigned long result = false;\
    asm ("mrs %0, ID_AA64MMFR0_EL1\n"\
         "ubfx %0, %0, #20, #4\n"\
         "cmp %0, #1\n"\
         "cset %0, eq\n"\
         : "+r" (result) :);\
    (bool)result;\
})
#define arch_mmu_is_64kb_granule_supported()\
({\
    unsigned long result = false;\
    asm ("mrs %0, ID_AA64MMFR0_EL1\n"\
         "ubfx %0, %0, #24, #4\n"\
         "cmp %0, #0\n"\
         "cset %0, eq\n"\
         : "+r" (result) :);\
    (bool)result;\
})

// Check if the MMU is enabled
#define arch_mmu_is_enabled()\
({\
    unsigned long result = false;\
    asm ("mrs %0, SCTLR_EL1\n"\
         "tst %0, #1\n"\
         "csetm %0, ne\n"\
         : "+r" (result) :);\
    (bool)result;\
})

// Emable or disable the MMu
void arch_mmu_enable(unsigned long ttbr0, unsigned long ttbr1, unsigned long mair, unsigned int page_size);

// This routine will perform a long jump to the kernel's virtual address space.
// It assumes kernel code is translated using TTBR1 and will clear TTBR0 after the jump
void arch_mmu_kernel_longjmp(uintptr_t pa_base, uintptr_t va_base);

// MMU context switching
#define arch_mmu_get_ttbr0()\
({\
    unsigned long result;\
    asm ("mrs %0, TTBR0_EL1"\
         : "=r" (result) :);\
    result;\
})
#define arch_mmu_get_ttbr1()\
({\
    unsigned long result;\
    asm ("mrs %0, TTBR1_EL1"\
         : "=r" (result) :);\
    result;\
})

void arch_mmu_set_ttbr0(uintptr_t ttb0, unsigned int asid);
void arch_mmu_clear_ttbr0(void);

// Given a VA, translates it to a PA using the current MMU context. Returns a valid PA or -1 if the translation cannot
// be performed
#define arch_mmu_translate_va(va)\
({\
    uintptr_t pa;\
    asm ("at S1E1R, %1\n"\
         "mrs x1, PAR_EL1\n"\
         "ubfx %0, x1, #12, #36\n"\
         "lsl %0, %0, #12\n"\
         "mvn x2, xzr\n"\
         "tst x1, #1\n"\
         "csel %0, %0, x2, eq\n"\
         : "+r" (pa)\
         : "r" (va)\
         : "x1", "x2");\
    pa;\
})

#endif // _ARCH_MMU_H_
