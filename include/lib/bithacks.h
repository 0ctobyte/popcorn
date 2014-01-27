#ifndef __BITHACKS_H__
#define __BITHACKS_H__

#include <stdint.h>

// Counts the number of bits set in bitmap which must be a 32-bit value
uint32_t bit_set_count(uint32_t bitmap);

// Returns the starting bit position of a contiguous set of width bits
// Returns a negative number if no such contiguous set exists in the bitmap
int32_t bit_find_contiguous_zeros(uint32_t bitmap, uint32_t width);

// Sets the bits starting from position index to position index+width-1
// Returns the modified bitmap
uint32_t bit_field_set(uint32_t bitmap, uint32_t index, uint32_t width);

#endif // __BITHACKS_H__

