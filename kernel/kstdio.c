#include <kernel/kstdio.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


int32_t uart0_putchar(const uint32_t c) {
	volatile uint32_t * const UART0DR = (uint32_t*)0x101F1000;
	*UART0DR = c;
	return(0);
}

int32_t putchar(uint32_t c) {
	return(uart0_putchar(c));
}

int32_t kputs(const char *s) {
	size_t len = strlen(s);
	for(int i = 0; i < len; i++) {
		putchar(s[i]);
	}
	return(0);
}

// This will be allocated on the data segment (BSS) rather than the stack!
char __kprintf_buffer[1024];
int32_t kprintf(const char *fmt, ...) {
	va_list args;
	int32_t r = 0;

	va_start(args, fmt);
	r = vsprintf(__kprintf_buffer, fmt, args);
	va_end(args);

	kputs(__kprintf_buffer);

	return(r);
}

