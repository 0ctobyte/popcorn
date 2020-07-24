/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/vm/vm_page.h>
#include <kernel/vm/vm_object.h>

vm_object_t kernel_object;
vm_object_t kernel_lva_object;

void vm_object_init(void) {
    lock_init(&kernel_object.lock);
    list_init(&kernel_object.ll_resident);
    vm_object_reference(&kernel_object);
    kernel_object.size = 0;

    lock_init(&kernel_lva_object.lock);
    list_init(&kernel_lva_object.ll_resident);
    vm_object_reference(&kernel_lva_object);
    kernel_lva_object.size = 0;

}

void vm_object_destroy(vm_object_t *object) {
    kassert(object != NULL);

    // FIXME clean up object when refcnt drops to zero
    lock_acquire_exclusive(&object->lock);
    object->refcnt--;
    lock_release_exclusive(&object->lock);
}

void vm_object_reference(vm_object_t *object) {
    kassert(object != NULL);

    lock_acquire_exclusive(&object->lock);
    object->refcnt++;
    lock_release_exclusive(&object->lock);
}

void vm_object_set_size(vm_object_t *object, size_t new_size) {
    kassert(object != NULL);

    lock_acquire_exclusive(&object->lock);
    if (new_size > object->size) object->size = new_size;
    lock_release_exclusive(&object->lock);
}
