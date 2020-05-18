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

#endif // __EVT_H__

