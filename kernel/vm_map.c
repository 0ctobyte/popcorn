#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kassert.h>
#include <string.h>

// Kernel vmap
vm_map_t kernel_vmap;

void vm_map_bootstrap(void) {
    vaddr_t kernel_virtual_start, kernel_virtual_end;

    pmap_virtual_space(&kernel_virtual_start, &kernel_virtual_end);

    // Initialize the kernel vm_map
    kernel_vmap.lock = SPINLOCK_INIT;
    kernel_vmap.pmap = pmap_kernel();
    kernel_vmap.start = kernel_virtual_start;
    kernel_vmap.end = max_kernel_virtual_end;
    kernel_vmap.size = 0;
    atomic_inc(&kernel_vmap.refcnt);

    // Add a mapping to the kernel_object for all of the kernel memory mapped to this point
    vm_mapping_t *initial_mapping = (vm_mapping_t*)pmap_steal_memory(sizeof(vm_mapping_t), &kernel_virtual_start, &kernel_virtual_end);
    initial_mapping->left = NULL;
    initial_mapping->right = NULL;
    initial_mapping->vstart = kernel_virtual_start;
    initial_mapping->vend = kernel_virtual_end;
    initial_mapping->prot = VM_PROT_ALL;
    initial_mapping->object = &kernel_object;
    initial_mapping->offset = 0;
}
