#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/evt.h>
#include <kernel/pmap.h>

#include <platform/interrupts.h>

void kmain(void) {
	kprintf("Initializing kernel...\n");

  // Setup the exception vector table
	evt_init();

  // Setup the IRQ system
	irq_init();
  
  // Initializes the pmap and pmm systems
  pmap_init();

	kprintf("what\n");
}

