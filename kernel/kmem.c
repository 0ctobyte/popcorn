#include <kernel/kmem.h>
#include <kernel/vm_km.h>
#include <kernel/slab.h>
#include <kernel/kassert.h>
#include <string.h>

// Slab to allocate slab_buf_t structs from
slab_t slab_buf_slab;

void kmem_init(void) {
}

void* kmem_alloc(size_t size) {
    return NULL;
}

void* kmem_zalloc(size_t size) {
    void *mem = kmem_alloc(size);
    if (mem != NULL) memset(mem, 0, size);
    return mem;
}

void kmem_free(void *mem, size_t size) {
}
