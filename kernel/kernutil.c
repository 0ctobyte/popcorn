#include <kernel/kernutil.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

volatile uint32_t * const UART0DR = (uint32_t*)0x101F1000;

int32_t uart0_putchar(const uint32_t c) {
	*UART0DR = c;
	return(0);
}

int32_t putchar(uint32_t c) {
	return(uart0_putchar(c));
}

int32_t puts(const char *s) {
	size_t len = strlen(s);
	for(int i = 0; i < len; i++) {
		putchar(s[i]);
	}
	return(0);
}

// Do we need separate buffers?
char __panic_buffer[1024];
void panic(const char *fmt, ...) {
	va_list args;
	int32_t r = 0;

	puts("\nPANIC: ");

	va_start(args, fmt);
	r = vsprintf(__panic_buffer, fmt, args);
	va_end(args);

	puts(__panic_buffer);

	for(;;);
}

// This will be allocated on the data segment (BSS) rather than the stack!
char __kprintf_buffer[1024];
int32_t kprintf(const char *fmt, ...) {
	va_list args;
	int32_t r = 0;

	va_start(args, fmt);
	r = vsprintf(__kprintf_buffer, fmt, args);
	va_end(args);

	puts(__kprintf_buffer);

	return(r);
}

