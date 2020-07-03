#include <kernel/vm/vm_page.h>
#include <kernel/vm/vm_object.h>

vm_object_t kernel_object;
vm_object_t kernel_lva_object;

void vm_object_init(void) {
    kernel_object.lock = SPINLOCK_INIT;
    kernel_object.ll_resident = LIST_INITIALIZER;
    atomic_inc(&kernel_object.refcnt);
    kernel_object.size = 0;

    kernel_lva_object.lock = SPINLOCK_INIT;
    kernel_lva_object.ll_resident = LIST_INITIALIZER;
    atomic_inc(&kernel_lva_object.refcnt);
    kernel_lva_object.size = 0;

}
