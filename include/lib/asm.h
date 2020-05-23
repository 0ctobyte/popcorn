#ifndef __ASM_H__
#define __ASM_H__

#include <stdint.h>

// Unsigned integer modulo
unsigned long _umod(unsigned long n, unsigned long m);

// Count trailing zeros
// Returns the # of continuous zeroes from the least significant bit
unsigned long _ctz(unsigned long n);

// Count leading zeros
// Returns the # of continuous zeros from the most significant bit
unsigned long _clz(unsigned long n);

// Reverse bits
// Reverses the bit order on the 32-bit integer
unsigned long _rbit(unsigned long n);

// Reverse byte order in a 32-bit or 64-bit word. Use to convert between big and little endian
unsigned long _rev32(unsigned long n);
unsigned long _rev64(unsigned long n);

// Counts the number of bits set
unsigned long _popcnt(unsigned long n);

#endif // __ASM_H__

