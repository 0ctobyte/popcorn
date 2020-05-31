#include <kernel/vm_object.h>

vm_object_t kernel_object;

void vm_object_bootstrap(void) {
    kernel_object.lock = SPINLOCK_INIT;
    kernel_object.resident_pages = NULL;
    atomic_inc(&kernel_object.refcnt);

    // This will be updated as kernel pages are added
    kernel_object.size = 0;
}
