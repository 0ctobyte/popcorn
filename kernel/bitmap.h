#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <sys/types.h>

typedef unsigned long bitmap_t;

// Returns the starting bit position of a contiguous set of width bits
// Returns a negative number if no such contiguous set exists in the bitmap
long bitmap_find_contiguous_zeros(bitmap_t bitmap, unsigned long width);

// Returns the starting bit position of a contiguous set of width bits aligned to a multiple of the given bit #
// Returns a negative number if no such contiguous set exists in the bitmap
long bitmap_find_contiguous_aligned_zeros(bitmap_t bitmap, unsigned long width, unsigned int alignment);

// Sets the bits starting from position index to position index+width-1
// Returns the modified bitmap
bitmap_t bitmap_field_set(bitmap_t bitmap, unsigned long index, unsigned long width);

// Clears the bits starting at position index to position index+width-1
// Returns the modified bitmap
bitmap_t bitmap_field_clear(bitmap_t bitmap, unsigned long index, unsigned long width);

#endif // _BITMAP_H_
