#ifndef __STRING_H__
#define __STRING_H__

#include <sys/types.h>

// Counts the number of characters in a string excluding the null termination
// character
size_t strlen(const char *str);

// Sets the first num bytes of a block of memory to the specified value
void* memset(void *ptr, uint8_t value, size_t num);

// Copies num bytes from src to dst. Don't use this function if src and dst
// overlap
void* memcpy(void *dst, const void *src, size_t num);

// Same as memcpy except this is safe to use when src and dst overlap
void* memmove(void* dst, const void *src, size_t num);

#endif // __STRING_H__

