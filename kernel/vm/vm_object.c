#include <kernel/kassert.h>
#include <kernel/vm/vm_page.h>
#include <kernel/vm/vm_object.h>

vm_object_t kernel_object;
vm_object_t kernel_lva_object;

void vm_object_init(void) {
    kernel_object.lock = SPINLOCK_INIT;
    kernel_object.ll_resident = LIST_INITIALIZER;
    vm_object_reference(&kernel_object);
    kernel_object.size = 0;

    kernel_lva_object.lock = SPINLOCK_INIT;
    kernel_lva_object.ll_resident = LIST_INITIALIZER;
    vm_object_reference(&kernel_lva_object);
    kernel_lva_object.size = 0;

}

void vm_object_destroy(vm_object_t *object) {
    kassert(object != NULL);

    // FIXME clean up object when refcnt drops to zero
    spinlock_writeacquire(&object->lock);
    object->refcnt--;
    spinlock_writerelease(&object->lock);
}

void vm_object_reference(vm_object_t *object) {
    kassert(object != NULL);

    spinlock_writeacquire(&object->lock);
    object->refcnt++;
    spinlock_writerelease(&object->lock);
}

void vm_object_set_size(vm_object_t *object, size_t new_size) {
    kassert(object != NULL);

    spinlock_writeacquire(&object->lock);
    if (new_size > object->size) object->size = new_size;
    spinlock_writerelease(&object->lock);
}
