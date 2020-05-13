#ifndef __ASM_H__
#define __ASM_H__

#include <stdint.h>

// Unsigned integer modulo
unsigned int _umod(unsigned int, unsigned int);

// Count trailing zeros
// Returns the # of continuous zeroes from the least significant bit
unsigned int _ctz(unsigned int);

// Count leading zeros
// Returns the # of continuous zeros from the most significant bit
unsigned int _clz(unsigned int);

// Reverse bits
// Reverses the bit order on the 32-bit integer
unsigned int _rbit(unsigned int);

#endif // __ASM_H__

