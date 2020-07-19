#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <kernel/serial.h>
#include <kernel/kstdio.h>

int kputs(const char *s) {
    size_t len = strlen(s);
    serial_write(s, len);

    return 0;
}

int kprintf(const char *fmt, ...) {
    char __kprintf_buffer[1024];
    va_list args;
    int r = 0;

    va_start(args, fmt);
    r = vsprintf(__kprintf_buffer, fmt, args);
    va_end(args);

    kputs(__kprintf_buffer);

    return r;
}
