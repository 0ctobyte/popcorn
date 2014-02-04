#ifndef __ASM_H__
#define __ASM_H__

#include <stdint.h>

// Count leading zeros
// Returns the # of continuous zeros from the most significant bit
uint32_t _clz(uint32_t);

// Reverse bits
// Reverses the bit order on the 32-bit integer
uint32_t _rbit(uint32_t);

#endif // __ASM_H__

