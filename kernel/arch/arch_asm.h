/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _ARCH_ASM_H_
#define _ARCH_ASM_H_

#include <sys/types.h>

// Unsigned integer modulo
#define arch_umod(n, m)\
({\
    unsigned long result = 0;\
    asm ("udiv x2, %1, %2\n"\
         "msub %0, x2, %2, %1\n"\
         : "=r" (result)\
         : "r" (n), "r" (m)\
         : "x2");\
    result;\
})

// Count trailing zeros
// Returns the # of continuous zeroes from the least significant bit
#define arch_ctz(n)\
({\
    unsigned long result = 0;\
    asm ("rbit %0, %1\n"\
         "clz %0, %0\n"\
         : "+r" (result)\
         : "r" (n));\
    result;\
})

// Count leading zeros
// Returns the # of continuous zeros from the most significant bit
#define arch_clz(n)\
({\
    unsigned long result = 0;\
    asm ("clz %0, %1\n"\
         : "=r" (result)\
         : "r" (n));\
    result;\
})

// Reverse bits
// Reverses the bit order on the 32-bit integer
#define arch_rbit(n)\
({\
    unsigned long result = 0;\
    asm ("rbit %0, %1\n"\
         : "=r" (result)\
         : "r" (n));\
    result;\
})

// Reverse byte order in a 32-bit or 64-bit word. Use to convert between big and little endian
#define arch_rev32(n)\
({\
    unsigned long result = 0;\
    asm ("rev %w0, %w1\n"\
         : "=r" (result)\
         : "r" (n));\
    result;\
})
#define arch_rev64(n)\
({\
    unsigned long result = 0;\
    asm ("rev %0, %1\n"\
         : "=r" (result)\
         : "r" (n));\
    result;\
})

// Counts the number of bits set
unsigned long arch_popcnt(unsigned long n);

// Fast versions of memset 0 and memmove
void arch_fast_move(void *dst_addr, void *src_addr, size_t num);
void arch_fast_zero(void *addr, size_t num);

#endif // _ARCH_ASM_H_
