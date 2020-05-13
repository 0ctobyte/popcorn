#ifndef __BITHACKS_H__
#define __BITHACKS_H__

#include <stdint.h>

// Counts the number of bits set in bitmap which must be a 32-bit value
unsigned int bit_set_count(unsigned int bitmap);

// Returns the starting bit position of a contiguous set of width bits
// Returns a negative number if no such contiguous set exists in the bitmap
int bit_find_contiguous_zeros(unsigned int bitmap, unsigned int width);

// Sets the bits starting from position index to position index+width-1
// Returns the modified bitmap
unsigned int bit_field_set(unsigned int bitmap, unsigned int index, unsigned int width);

#endif // __BITHACKS_H__

