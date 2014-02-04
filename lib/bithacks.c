#include <lib/bithacks.h>
#include <lib/asm.h>

// Counts the number of bits set in bitmap which must be a 32-bit value
// This algorithm was acquired from here:
// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
uint32_t bit_set_count(uint32_t bitmap) {
	register uint32_t v = bitmap;
	v = v - ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return((((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24);
}

// Returns the starting bit position of a contiguous set of width bits
// Returns a negative number if no such contiguous set exists in the bitmap
int32_t bit_find_contiguous_zeros(uint32_t bitmap, uint32_t width) {
	if(bitmap == 0) return(0);
	if(width > 32) return(-1);

	// Reverse the bit order of the bitmap, so _clz (count leading zeros) will
	// in fact count the trailing zeros
	register uint32_t v = _rbit(bitmap);
	register int32_t index;

	for(index = 0; index < 32;) {
		// Count the leading zeros in the bitmap
		uint32_t num = _clz(v);
		if(num == 0) {
			// If there are none, count the leading 1's
			// we will need to shift these 1's out of the bitmap
			num = _clz(~v);
		} else if(num >= width) {
			// If there are enough zeros, break out of the loop
			break;
		}

		// Not enough zeros or leading 1's, so shift them out of the bitmap
		v <<= num;
		// Set the right-padded zeros, due to the shift, to 1 so that
		// they are not counted when calling clz
		v |= ((1 << num) - 1);
		// Increment the index by the shifted amount
		index += num;
	}

	return(index);
}

// Sets the bits starting at position index to position index+width-1
// Returns the modified bitmap
uint32_t bit_field_set(uint32_t bitmap, uint32_t index, uint32_t width) {
	// Shift the bitmap right so that the bit at position index is now at
	// position zero
	register uint32_t v = bitmap >> index;
	// Set the low order width bits to 1
	v |= ((1 << width) - 1);
	// Now shift v back and or with bitmap
	return((v << index) | bitmap);
}

