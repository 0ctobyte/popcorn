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
	volatile uint32_t *KMIBASE = (uint32_t*) 0x10006000;
	char s[2];
	s[0] = (char)*(KMIBASE+2);
	s[1] = '\n';
	print_uart0(s);
}

void kmain(void) {
	irq_init();
	bool i = interrupts_enabled();
	if(i)
		print_uart0("true\n");
	else
		print_uart0("false\n");

	volatile uint32_t *KMIBASE = (uint32_t*) 0x10006000;
	*KMIBASE |= ((1 << 4) | (1 << 2)); // Enable RX interrupt
	enable_irq(IRQ_KMI0);
	register_isr(IRQ_KMI0, keyboard_isr);
}

