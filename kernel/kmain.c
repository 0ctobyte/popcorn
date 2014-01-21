#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>

#include <sys/interrupts.h>

extern void __kernel_virtual_start();
extern void __kernel_physical_start();
extern void __kernel_virtual_end();
extern void __kernel_physical_end();

void keyboard_isr(void) {
	volatile uint32_t *KMIBASE = (uint32_t*) 0x10006000;
	char c;
	c = (char)*(KMIBASE+2);
	kputc(c);
}

void kmain(void) {
	vectors_install(); // Copy the vector table to address 0x0
	irq_init(); // Setup the IRQ system

	kprintf("__kernel_virtual_start: %#p\n __kernel_virtual_end: %#p\n __kernel_physical_start: %#p\n __kernel_physical_end: %#p\n", &__kernel_virtual_start, 
			&__kernel_virtual_end, &__kernel_physical_start, &__kernel_physical_end);
	kassert(0 < 1);

	bool i = interrupts_enabled();
	if(i)
		kputs("true\n");
	else
		kputs("false\n");

	volatile uint32_t *KMIBASE = (uint32_t*) 0x10006000;
	*KMIBASE |= ((1 << 4) | (1 << 2)); // Enable RX interrupt
	enable_irq(IRQ_KMI0);
	register_isr(IRQ_KMI0, keyboard_isr);
}

