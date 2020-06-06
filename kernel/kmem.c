#include <kernel/kmem.h>
#include <kernel/vm_km.h>
#include <kernel/slab.h>
#include <kernel/kassert.h>
#include <lib/asm.h>

// Slab to allocate slab_buf_t structs from
slab_t slab_buf_slab;

void kmem_init(void) {
}

void* kmem_alloc(size_t size) {
    return NULL;
}

void* kmem_zalloc(size_t size) {
    void *mem = kmem_alloc(size);
    if (mem != NULL) _fast_zero((uintptr_t)mem, size);
    return mem;
}

void kmem_free(void *mem, size_t size) {
}
