#ifndef _KSTDIO_H_
#define _KSTDIO_H_

#include <sys/types.h>

long kputc(const char c);
long kputs(const char *s);
long kprintf(const char *fmt, ...);

#endif // _KSTDIO_H_
