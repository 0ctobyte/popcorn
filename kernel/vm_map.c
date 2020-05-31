#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kassert.h>
#include <string.h>

// Kernel vmap
vm_map_t kernel_vmap;

void vm_map_bootstrap(void) {
    vaddr_t kernel_virtual_start, kernel_virtual_end;

    pmap_virtual_space(&kernel_virtual_start, &kernel_virtual_end);

    kernel_vmap.lock = SPINLOCK_INIT;
    kernel_vmap.pmap = pmap_kernel();
    kernel_vmap.start = kernel_virtual_start;
    kernel_vmap.end = max_kernel_virtual_end;
    kernel_vmap.size = max_kernel_virtual_end - kernel_virtual_start;
}
