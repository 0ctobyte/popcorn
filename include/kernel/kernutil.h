#ifndef __KERNUTIL_H__
#define __KERNUTIL_H__

#include <stdint.h>

int32_t kprintf(const char *fmt, ...);
int32_t puts(const char *s);
void panic(const char *fmt, ...);

#endif // __KERNUTIL_H__

