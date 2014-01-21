#ifndef __KSTDIO_H__
#define __KSTDIO_H__

#include <sys/types.h>

int32_t kputc(const char c);
int32_t kputs(const char *s);
int32_t kprintf(const char *fmt, ...);

#endif // __KSTDIO_H__

