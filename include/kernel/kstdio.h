#ifndef __KSTDIO_H__
#define __KSTDIO_H__

#include <sys/types.h>

long kputc(const char c);
long kputs(const char *s);
long kprintf(const char *fmt, ...);

#endif // __KSTDIO_H__

