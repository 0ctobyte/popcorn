#ifndef __ASM_H__
#define __ASM_H__

#include <stdint.h>

// Unsigned integer modulo
unsigned long _umod(unsigned long, unsigned long);

// Count trailing zeros
// Returns the # of continuous zeroes from the least significant bit
unsigned long _ctz(unsigned long);

// Count leading zeros
// Returns the # of continuous zeros from the most significant bit
unsigned long _clz(unsigned long);

// Reverse bits
// Reverses the bit order on the 32-bit integer
unsigned long _rbit(unsigned long);

#endif // __ASM_H__

