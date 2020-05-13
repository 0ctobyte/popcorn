#include <stdio.h>

int sprintf(char *s, const char *fmt, ...) {
    va_list args;
    int i = 0;

    va_start(args, fmt);
    i = vsprintf(s, fmt, args);
    va_end(args);

    return(i);
}

