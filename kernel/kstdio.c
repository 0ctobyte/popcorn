#include <kernel/kstdio.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int32_t kputc(const char c) {
	putchar(c);
	return(c);
}

int32_t kputs(const char *s) {
	size_t len = strlen(s);
	for(uint32_t i = 0; i < len; i++) {
		kputc(s[i]);
	}
	return(0);
}

// This will be allocated on the data segment (BSS) rather than the stack!
int32_t kprintf(const char *fmt, ...) {
  char __kprintf_buffer[1024];
	va_list args;
	int32_t r = 0;

	va_start(args, fmt);
	r = vsprintf(__kprintf_buffer, fmt, args);
	va_end(args);

	kputs(__kprintf_buffer);

	return(r);
}

