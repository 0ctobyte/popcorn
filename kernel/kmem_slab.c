#include <kernel/kassert.h>
#include <kernel/spinlock.h>
#include <kernel/kstdio.h>
#include <kernel/list.h>
#include <kernel/slab.h>
#include <kernel/vm/vm_km.h>
#include <kernel/kmem_slab.h>

void kmem_slab_create_no_vm(kmem_slab_t *kmem_slab, size_t object_size, size_t num_objects, void *buf) {
    kassert(kmem_slab != NULL);

    kmem_slab->lock = SPINLOCK_INIT;
    kmem_slab->size = object_size * num_objects;

    slab_init(&kmem_slab->slab, &kmem_slab->slab_buf, buf, kmem_slab->size, object_size);
}

void kmem_slab_create(kmem_slab_t *kmem_slab, size_t object_size, size_t num_objects) {
    kassert(kmem_slab != NULL);

    kmem_slab->lock = SPINLOCK_INIT;
    kmem_slab->size = object_size * num_objects;

    void *buf = (void*)vm_km_alloc(kmem_slab->size, VM_KM_FLAGS_WIRED);
    slab_init(&kmem_slab->slab, &kmem_slab->slab_buf, buf, kmem_slab->size, object_size);
}

void kmem_slab_destroy(kmem_slab_t *kmem_slab) {
    kassert(kmem_slab != NULL && slab_buf_is_full(&kmem_slab->slab_buf));

    spinlock_acquire(&kmem_slab->lock);
    vm_km_free((vaddr_t)kmem_slab->slab_buf.buf, kmem_slab->size, 0);
    spinlock_release(&kmem_slab->lock);
}

void* kmem_slab_alloc(kmem_slab_t *kmem_slab) {
    kassert(kmem_slab != NULL);

    spinlock_acquire(&kmem_slab->lock);
    void *ptr = slab_alloc(&kmem_slab->slab);
    spinlock_release(&kmem_slab->lock);

    return ptr;
}

void* kmem_slab_zalloc(kmem_slab_t *kmem_slab) {
    kassert(kmem_slab != NULL);

    spinlock_acquire(&kmem_slab->lock);
    void *ptr = slab_zalloc(&kmem_slab->slab);
    spinlock_release(&kmem_slab->lock);

    return ptr;
}

void kmem_slab_free(kmem_slab_t *kmem_slab, void *ptr) {
    kassert(kmem_slab != NULL && ptr != NULL);

    spinlock_acquire(&kmem_slab->lock);
    slab_free(&kmem_slab->slab, ptr);
    spinlock_release(&kmem_slab->lock);
}
