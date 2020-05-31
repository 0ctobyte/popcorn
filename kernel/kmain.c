#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/exceptions.h>
#include <kernel/interrupts.h>
#include <kernel/devicetree.h>
#include <kernel/vmm.h>

#include <lib/asm.h>

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;


void kmain(void) {
    // Setup the exception vector table
    exceptions_init();

    if (!devicetree_find_memory(&MEMBASEADDR, &MEMSIZE)) HALT();

    kprintf("Hello World\n");

    vmm_init();

    kprintf("vmm_init() - done!\n");
}
