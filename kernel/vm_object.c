#include <kernel/vm_object.h>

vm_object_t kernel_object;

extern void _vm_page_insert_in_object(vm_page_t *pages, size_t num_pages, vm_object_t *object, vm_offset_t starting_offset);

void vm_object_bootstrap(paddr_t kernel_physical_start, paddr_t kernel_physical_end) {
    kernel_object.lock = SPINLOCK_INIT;
    kernel_object.ll_resident_pages = LIST_INITIALIZER;
    atomic_inc(&kernel_object.refcnt);
    kernel_object.size = 0;

    // Add all the pages allocated for the kernel up to this point
    size_t num_pages = (kernel_physical_end - kernel_physical_start) >> PAGESHIFT;
    vm_page_t *pages = vm_page_from_pa(kernel_physical_start);
    _vm_page_insert_in_object(pages, num_pages, &kernel_object, 0);
}
