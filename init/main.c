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

void keyboard_isr(void) {
}

void kmain(void) {
	irq_init();
	bool i = interrupts_enabled();
	if(i)
		print_uart0("true\n");
	else
		print_uart0("false\n");

	

	enable_irq(IRQ_KMI0);
	register_isr(IRQ_KMI0, keyboard_isr);
}

