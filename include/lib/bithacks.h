#ifndef __BITHACKS_H__
#define __BITHACKS_H__

#include <stdint.h>

// Counts the number of bits set in bitmap which must be a 32-bit value
unsigned long bit_set_count(unsigned long bitmap);

// Returns the starting bit position of a contiguous set of width bits
// Returns a negative number if no such contiguous set exists in the bitmap
long bit_find_contiguous_zeros(unsigned long bitmap, unsigned long width);

// Sets the bits starting from position index to position index+width-1
// Returns the modified bitmap
unsigned long bit_field_set(unsigned long bitmap, unsigned long index, unsigned long width);

#endif // __BITHACKS_H__

