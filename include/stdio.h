#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <sys/types.h>

int putchar(char c);
int sprintf(char *s, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);

#endif // __STDIO_H__

