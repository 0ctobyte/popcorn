#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <sys/types.h>

int32_t putchar(char c);
int32_t sprintf(char *s, const char *fmt, ...);
int32_t vsprintf(char *buf, const char *fmt, va_list args);

#endif // __STDIO_H__

