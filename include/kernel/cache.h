#ifndef __CACHE_H__
#define __CACHE_H__

// Invalidates all TLB entries
void tlb_invalidate_all(void);

// Invalidates the icache completely to the polong of unification
void icache_invalidate_all(void);

// Invalidates ever level of dcache completely by set/way
void dcache_invalidate_all(void);

#endif // __CACHE_H__

