#include <kernel/kstdio.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int kputc(const char c) {
    while(putchar(c) == -1) continue;

    // Send a carriage return after a newline
    if(c == '\n') while(putchar('\r') == -1) continue;

    return(c);
}

int kputs(const char *s) {
    size_t len = strlen(s);
    for(unsigned int i = 0; i < len; i++) {
        kputc(s[i]);
    }
    return(0);
}

// This will be allocated on the data segment (BSS) rather than the stack!
int kprintf(const char *fmt, ...) {
    char __kprintf_buffer[1024];
    va_list args;
    int r = 0;

    va_start(args, fmt);
    r = vsprintf(__kprintf_buffer, fmt, args);
    va_end(args);

    kputs(__kprintf_buffer);

    return(r);
}
