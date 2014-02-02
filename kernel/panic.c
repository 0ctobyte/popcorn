#include <kernel/panic.h>
#include <kernel/kstdio.h>

#include <mach/interrupts.h>

#include <stdarg.h>
#include <stdio.h>

char __panic_buffer[1024];
void panic(const char *fmt, ...) {
	va_list args;
	int32_t r = 0;

	interrupts_disable();

	kputs("\nPANIC: ");

	va_start(args, fmt);
	r = vsprintf(__panic_buffer, fmt, args);
	va_end(args);

	kputs(__panic_buffer);

	HALT();
}

