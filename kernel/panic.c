#include <kernel/panic.h>
#include <kernel/kstdio.h>
#include <kernel/interrupts.h>

#include <stdarg.h>
#include <stdio.h>

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

