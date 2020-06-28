#include <limits.h>
#include <kernel/arch/asm.h>
#include <kernel/bitmap.h>

#define BITS (sizeof(bitmap_t) * CHAR_BIT)

// Returns the starting bit position of a contiguous set of width bits
// Returns a negative number if no such contiguous set exists in the bitmap
long bitmap_find_contiguous_zeros(bitmap_t bitmap, unsigned long width) {
    return bitmap_find_contiguous_aligned_zeros(bitmap, width, 1);
}

// Returns the starting bit position of a contiguous set of width bits aligned to a multiple of the given bit #
// Returns a negative number if no such contiguous set exists in the bitmap
long bitmap_find_contiguous_aligned_zeros(bitmap_t bitmap, unsigned long width, unsigned int alignment) {
    if(bitmap == 0) return 0;
    if(width > BITS) return -1;

    // Reverse the bit order of the bitmap, so _clz (count leading zeros) will in fact count the trailing zeros
    register bitmap_t v = _rbit(bitmap);
    register long index;

    for(index = 0; index < BITS;) {
        // Count the leading zeros in the bitmap
        bitmap_t num = _clz(v);
        if(num == 0) {
            // If there are none, count the leading 1's we will need to shift these 1's out of the bitmap
            // Align the number of bits to shift out so we're always only checking for contiguous zero bits where the first bit is aligned
            num = _clz(~v);
            unsigned long remainder = _umod(num, alignment);
            if (remainder != 0) num += (alignment - remainder);
        } else if(num >= width) {
            // If there are enough zeros, break out of the loop
            break;
        }

        // Not enough zeros or leading 1's, so shift them out of the bitmap
        v <<= num;
        // Set the right-padded zeros, due to the shift, to 1 so that they are not counted when calling clz
        v |= ((1 << num) - 1);
        // Increment the index by the shifted amount
        index += num;
    }

    return index;
}

// Sets the bits starting at position index to position index+width-1
// Returns the modified bitmap
bitmap_t bitmap_field_set(bitmap_t bitmap, unsigned long index, unsigned long width) {
    register unsigned long mask = (1 << width) - 1;
    return (bitmap | (mask << index));
}

// Clears the bits starting at position index to position index+width-1
// Returns the modified bitmap
bitmap_t bitmap_field_clear(bitmap_t bitmap, unsigned long index, unsigned long width) {
    register unsigned long mask = (1 << width) - 1;
    return (bitmap & ~(mask << index));
}
