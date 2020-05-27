#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/exceptions.h>
#include <kernel/interrupts.h>
#include <kernel/pmap.h>
#include <kernel/devicetree.h>

#include <lib/asm.h>

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;

void kmain(void) {
    // Setup the exception vector table
    exceptions_init();

    if (!devicetree_find_memory(&MEMBASEADDR, &MEMSIZE)) HALT();

    kprintf("Hello World\n");

    // Initializes the pmap and pmm systems
    // We need to also map device memory here (like the GIC and UART)
    // We need to map the GIC in order to set up the IRQ system
    // We need to map UART in order to use kprintf!
    pmap_init();
    kprintf("pmap_init() - done!\n");
    vaddr_t vstartp, vendp;
    pmap_virtual_space(&vstartp, &vendp);
    kprintf("virtual space: %p - %p\n", vstartp, vendp);
}
