#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kassert.h>
#include <string.h>

// Kernel vmap
vm_map_t kernel_vmap;

void _vm_mapping_remove_all(vm_mapping_t *root) {
    // FIXME Implement this
}

void vm_map_init(void) {
    // FIXME Init vm_map module
}

vm_map_t* vm_map_create(pmap_t *pmap, vaddr_t vmin, vaddr_t vmax) {
    // FIXME Allocate map
    vm_map_t *map;

    map->lock = SPINLOCK_INIT;
    map->pmap = pmap;
    map->start = vmin;
    map->end = vmax;
    map->size = 0;
    map->refcnt = 0;

    return map;
}

void vm_map_destroy(vm_map_t *vmap) {
    atomic_dec(&vmap->refcnt);

    if (vmap->refcnt == 0) {
        // FIXME Free map
        _vm_mapping_remove_all(vmap->root);
    }
}
