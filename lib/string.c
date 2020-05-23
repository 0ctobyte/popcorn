#include <string.h>

size_t strlen(const char *str) {
    size_t len = 0;
    for(len = 0; *str++ != '\0'; len++);
    return(len);
}

int strcmp(const char *str1, const char *str2) {
    unsigned int i;
    for (i = 0; str1[i] != '\0' && str2[i] != '\0' && str1[i] == str2[i]; i++);
    return (str1[i] == '\0' && str2[i] == '\0') ? 0 : str1[i] - str2[i];
}

int strncmp(const char *str1, const char *str2, size_t num) {
    unsigned int i;
    for (i = 0; i < num && str1[i] == str2[i]; i++);
    return (i == num) ? 0 : str1[i] - str2[i];
}

const char* strchr(const char *str, int character) {
    size_t len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == character) return str + i;
    }
    return NULL;
}

void* memset(void *ptr, uint8_t value, size_t num) {
    uint8_t *buf = ptr;
    for(unsigned long i = 0; i < num; ++i) *buf++ = value;
    return(ptr);
}

void* memcpy(void *dst, const void *src, size_t num) {
    const unsigned long *s = src;
    for(unsigned long i = 0, *buf = dst; i < num; ++i) *buf++ = *s++;
    return(dst);
}

void* memmove(void *dst, const void *src, size_t num) {
    // It is only a problem when the dst is at a higher address and overlaps src
    if(((unsigned long*)dst > (const unsigned long*)src && (unsigned long*)dst < ((const unsigned long*)src+num))) {
        const unsigned long *s = (const unsigned long*)src+num;
        for(unsigned long i = 0, *buf = (unsigned long*)dst+num; i < num; ++i)
            *buf-- = *s--;
        return(dst);
    } else {
        return(memcpy(dst, src, num));
    }
}
