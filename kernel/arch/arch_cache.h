/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _ARCH_CACHE_H_
#define _ARCH_CACHE_H_

#include <sys/types.h>

// Invalidates the icache completely to the polong of unification
#define arch_icache_invalidate_all()\
({\
    asm volatile ("ic ialluis\n"\
                  "dsb sy\n"\
                  "isb sy\n");\
})

// Flushes every level of dcache completely by set/way
void arch_dcache_flush_all(void);

#endif // _ARCH_CACHE_H_
