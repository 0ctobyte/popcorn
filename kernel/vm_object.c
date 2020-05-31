#include <kernel/vm_object.h>

vm_object_t kernel_object;

void vm_object_bootstrap(paddr_t kernel_physical_start, paddr_t kernel_physical_end) {
    kernel_object.lock = SPINLOCK_INIT;
    kernel_object.resident_pages = NULL;
    atomic_inc(&kernel_object.refcnt);
    kernel_object.size = kernel_physical_end - kernel_physical_start;

    // Add all the pages allocated for the kernel up to this point
    for (unsigned long offset = 0; offset < kernel_object.size; offset += PAGESIZE) {
        vm_page_t *page = vm_page_from_pa(kernel_physical_start + offset);
        page->object = &kernel_object;
        page->offset = offset;

        vm_page_t *next = kernel_object.resident_pages;
        page->next_resident = next;
        if (next != NULL) next->prev_resident = page;
        page->prev_resident = NULL;
        kernel_object.resident_pages = page;
    }
}
