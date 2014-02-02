#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/evt.h>
#include <kernel/pmap.h>

#include <mach/interrupts.h>

extern uintptr_t __kernel_virtual_start;
extern uintptr_t __kernel_physical_start;
extern uintptr_t __kernel_virtual_end;
extern uintptr_t __kernel_physical_end;

void kmain(void) {
	evt_init(); // Setup the exception vector table
	irq_init(); // Setup the IRQ system
	pmap_init();
	//kprintf("__kernel_virtual_start: %#p\n __kernel_virtual_end: %#p\n __kernel_physical_start: %#p\n __kernel_physical_end: %#p\n", &__kernel_virtual_start, &__kernel_virtual_end, &__kernel_physical_start, &__kernel_physical_end);
}

