#ifndef __MMU_H__
#define __MMU_H__

// MMU feature support checkers
bool mmu_is_4kb_granule_supported(void);
bool mmu_is_16kb_granule_supported(void);
bool mmu_is_64kb_granule_supported(void);

// Check if the MMU is enabled
bool mmu_is_enabled(void);

// Emable or disable the MMu
void mmu_enable(unsigned long ttbr0, unsigned long ttbr1, unsigned long mair, unsigned int page_size);

// This routine will perform a long jump to the kernel's virtual address space.
// It assumes kernel code is translated using TTBR1 and will clear TTBR0 after the jump
void mmu_kernel_longjmp(uintptr_t pa_base, uintptr_t va_base);

#endif // __EVT_H__

