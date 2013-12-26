#include <stdint.h>
#include <stdbool.h>

#include "../include/kernel/interrupts.h"

volatile uint32_t * const UART0DR = (uint32_t*) 0x101f1000;

void print_uart0(const char *s) {
	while(*s != '\0') {
		*UART0DR = (uint32_t)(*s);
		s++;
	}
}


void kmain(void) {
	irq_init();
	print_uart0("Hello World!\n");
}

