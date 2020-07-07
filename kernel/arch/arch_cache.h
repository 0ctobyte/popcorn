#ifndef _ARCH_CACHE_H_
#define _ARCH_CACHE_H_

#include <sys/types.h>

// Invalidates the icache completely to the polong of unification
void arch_icache_invalidate_all(void);

// Flushes every level of dcache completely by set/way
void arch_dcache_flush_all(void);

#endif // _ARCH_CACHE_H_