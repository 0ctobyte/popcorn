#ifndef _ARCH_ASM_H_
#define _ARCH_ASM_H_

#include <sys/types.h>

// Unsigned integer modulo
unsigned long arch_umod(unsigned long n, unsigned long m);

// Count trailing zeros
// Returns the # of continuous zeroes from the least significant bit
unsigned long arch_ctz(unsigned long n);

// Count leading zeros
// Returns the # of continuous zeros from the most significant bit
unsigned long arch_clz(unsigned long n);

// Reverse bits
// Reverses the bit order on the 32-bit integer
unsigned long arch_rbit(unsigned long n);

// Reverse byte order in a 32-bit or 64-bit word. Use to convert between big and little endian
unsigned long arch_rev32(unsigned long n);
unsigned long arch_rev64(unsigned long n);

// Counts the number of bits set
unsigned long arch_popcnt(unsigned long n);

// Fast versions of memset 0 and memmove
void arch_fast_move(uintptr_t dst_addr, uintptr_t src_addr, size_t num);
void arch_fast_zero(uintptr_t addr, size_t num);

#endif // _ARCH_ASM_H_
