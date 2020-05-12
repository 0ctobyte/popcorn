#include <string.h>

size_t strlen(const char *str) {
    size_t len = 0;
    for(len = 0; *str++ != '\0'; len++);
    return(len);
}


void* memset(void *ptr, uint8_t value, size_t num) {
    uint8_t *buf = ptr;
    for(uint32_t i = 0; i < num; ++i) *buf++ = value;
    return(ptr);
}

void* memcpy(void *dst, const void *src, size_t num) {
    const uint32_t *s = src;
    for(uint32_t i = 0, *buf = dst; i < num; ++i) *buf++ = *s++;
    return(dst);
}

void* memmove(void *dst, const void *src, size_t num) {
    // It is only a problem when the dst is at a higher address and overlaps
    // src
    if(((uint32_t*)dst > (uint32_t*)src && (uint32_t*)dst <
            ((uint32_t*)src+num))) {
        const uint32_t *s = (uint32_t*)src+num;
        for(uint32_t i = 0, *buf = (uint32_t*)dst+num; i < num; ++i)
            *buf-- = *s--;
        return(dst);
    } else {
        return(memcpy(dst, src, num));
    }
}
