#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/evt.h>
#include <kernel/pmap.h>

#include <mach/interrupts.h>

void kmain(void) {
	kprintf("Initializing kernel...\n");
	evt_init(); // Setup the exception vector table
	irq_init(); // Setup the IRQ system
	pmap_init();

	kprintf("what\n");
}

