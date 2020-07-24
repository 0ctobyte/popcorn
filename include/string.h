/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __STRING_H__
#define __STRING_H__

#include <sys/types.h>

// Counts the number of characters in a string excluding the null termination character
size_t strlen(const char *str);

// Compares two null-terminated strings. Returns 0 if equal, <0 if the first character that does not match
// is has a lower value in str1 than in str2. Returns >0 if the first character that does not match
// has a higher value in str1 than in str2
int strcmp(const char *str1, const char *str2);

// Compares two strings up to num characters. Returns 0 if all num characters are equal otherwise returns <0 if
// the first character that does not match has a lower value in str1 than in str2. Returns >0 if the first character
// that does not match has a higher value in str1 than in str2.
int strncmp(const char *str1, const char *str2, size_t num);

// Finds the first occurrence of the character in the string. Returns the pointer to that character in the string
// otherwise returns NULL
const char* strchr(const char *str, int character);

// Sets the first num bytes of a block of memory to the specified value
void* memset(void *ptr, uint8_t value, size_t num);

// Copies num bytes from src to dst. Don't use this function if src and dst overlap
void* memcpy(void *dst, const void *src, size_t num);

// Same as memcpy except this is safe to use when src and dst overlap
void* memmove(void* dst, const void *src, size_t num);

#endif // __STRING_H__
