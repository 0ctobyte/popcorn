#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/exceptions.h>
#include <kernel/interrupts.h>
#include <kernel/devicetree.h>
#include <kernel/vmm.h>
#include <kernel/vm_map.h>
#include <kernel/vm_object.h>
#include <kernel/vm_km.h>

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

    kprintf("vmap - start = %p, end = %p\n", vm_map_kernel()->start, vm_map_kernel()->end);

    vaddr_t new_va = vm_km_alloc(PAGESIZE, VM_KM_FLAGS_WIRED | VM_KM_FLAGS_ZERO);
    kprintf("new_va = %p\n", new_va);

    for (vm_mapping_t *mapping = vm_map_kernel()->mappings; mapping != NULL; mapping = mapping->next) {
        kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n", mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
    }
}
