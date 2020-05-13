#ifndef __KSTDIO_H__
#define __KSTDIO_H__

#include <sys/types.h>

int kputc(const char c);
int kputs(const char *s);
int kprintf(const char *fmt, ...);

#endif // __KSTDIO_H__

