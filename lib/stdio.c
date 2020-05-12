#include <stdio.h>

int32_t sprintf(char *s, const char *fmt, ...) {
    va_list args;
    int32_t i = 0;

    va_start(args, fmt);
    i = vsprintf(s, fmt, args);
    va_end(args);

    return(i);
}

