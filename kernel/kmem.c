#include <kernel/kmem.h>
#include <kernel/vm_km.h>
#include <kernel/slab.h>
#include <kernel/kassert.h>

// Slab to allocate slab_buf_t structs from
slab_t slab_buf_slab;

void kmem_init(void) {
}
