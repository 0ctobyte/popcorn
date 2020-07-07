#ifndef _ARCH_MMU_H_
#define _ARCH_MMU_H_

#include <sys/types.h>

// Invalidates all TLB entries
void arch_tlb_invalidate_all(void);

// Invalidate VA for only entries with the specified ASID
void arch_tlb_invalidate_va(uintptr_t va, unsigned int asid);

// Invalidate all entries corresponding to the specified ASID
void arch_tlb_invalidate_asid(unsigned int asid);

// MMU feature support checkers
bool arch_mmu_is_4kb_granule_supported(void);
bool arch_mmu_is_16kb_granule_supported(void);
bool arch_mmu_is_64kb_granule_supported(void);

// Check if the MMU is enabled
bool arch_mmu_is_enabled(void);

// Emable or disable the MMu
void arch_mmu_enable(unsigned long ttbr0, unsigned long ttbr1, unsigned long mair, unsigned int page_size);

// This routine will perform a long jump to the kernel's virtual address space.
// It assumes kernel code is translated using TTBR1 and will clear TTBR0 after the jump
void arch_mmu_kernel_longjmp(uintptr_t pa_base, uintptr_t va_base);

// MMU context switching
unsigned long arch_mmu_get_ttbr0(void);
unsigned long arch_mmu_get_ttbr1(void);
void arch_mmu_set_ttbr0(uintptr_t ttb0, unsigned int asid);
void arch_mmu_clear_ttbr0(void);

// Given a VA, translates it to a PA using the current MMU context. Returns a valid PA or -1 if the translation cannot be performed
uintptr_t arch_mmu_translate_va(uintptr_t va);

#endif // _ARCH_MMU_H_
