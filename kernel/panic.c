#include <stdarg.h>
#include <stdio.h>
#include <kernel/kstdio.h>
#include <kernel/arch/interrupts.h>
#include <kernel/panic.h>

void __attribute__((noreturn)) panic(const char *fmt, ...) {
    char __panic_buffer[1024];
    va_list args;

    interrupts_disable();

    kputs("\nPANIC: ");

    va_start(args, fmt);
    vsprintf(__panic_buffer, fmt, args);
    va_end(args);

    kputs(__panic_buffer);

    HALT();
}
