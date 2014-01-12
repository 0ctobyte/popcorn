#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stdint.h>

int32_t sprintf(char *s, const char *fmt, ...);
int32_t vsprintf(char *buf, const char *fmt, va_list args);

#endif

