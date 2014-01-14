#ifndef __KERNUTIL_H__
#define __KERNUTIL_H__

#include <sys/types.h>

int32_t kprintf(const char *fmt, ...);
int32_t puts(const char *s);
void panic(const char *fmt, ...);

#define kassert(b) (b) ? (void)0 : \
	panic("Assertion Failed: %s:%d: '%s'\n", __FILE__, __LINE__, #b);

#endif // __KERNUTIL_H__

