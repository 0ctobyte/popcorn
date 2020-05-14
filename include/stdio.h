#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <sys/types.h>

long putchar(char c);
long sprintf(char *s, const char *fmt, ...);
long vsprintf(char *buf, const char *fmt, va_list args);

#endif // __STDIO_H__

