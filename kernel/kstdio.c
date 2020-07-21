#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <kernel/kresult.h>
#include <kernel/console.h>
#include <kernel/kstdio.h>

int kputs(const char *s) {
    size_t len = strlen(s), count = 0;
    kresult_t res;

    do {
        size_t c = 0;
        res = console_write(s + count, len, &c);
        count += c;
    } while (res = KRESULT_OK && count < len);

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
