#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/evt.h>
#include <kernel/pmap.h>

#include <mach/interrupts.h>

extern void __kernel_virtual_start();
extern void __kernel_physical_start();
extern void __kernel_virtual_end();
extern void __kernel_physical_end();

void kmain(void) {
	evt_init(); // Setup the exception vector table
	irq_init(); // Setup the IRQ system

	kprintf("__kernel_virtual_start: %#p\n __kernel_virtual_end: %#p\n __kernel_physical_start: %#p\n __kernel_physical_end: %#p\n", &__kernel_virtual_start, 
			&__kernel_virtual_end, &__kernel_physical_start, &__kernel_physical_end);
}

