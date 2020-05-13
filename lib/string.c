#include <string.h>

size_t strlen(const char *str) {
    size_t len = 0;
    for(len = 0; *str++ != '\0'; len++);
    return(len);
}


void* memset(void *ptr, uint8_t value, size_t num) {
    uint8_t *buf = ptr;
    for(unsigned int i = 0; i < num; ++i) *buf++ = value;
    return(ptr);
}

void* memcpy(void *dst, const void *src, size_t num) {
    const unsigned int *s = src;
    for(unsigned int i = 0, *buf = dst; i < num; ++i) *buf++ = *s++;
    return(dst);
}

void* memmove(void *dst, const void *src, size_t num) {
    // It is only a problem when the dst is at a higher address and overlaps src
    if(((unsigned int*)dst > (const unsigned int*)src && (unsigned int*)dst < ((const unsigned int*)src+num))) {
        const unsigned int *s = (const unsigned int*)src+num;
        for(unsigned int i = 0, *buf = (unsigned int*)dst+num; i < num; ++i)
            *buf-- = *s--;
        return(dst);
    } else {
        return(memcpy(dst, src, num));
    }
}
