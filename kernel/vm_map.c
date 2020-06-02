#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kassert.h>
#include <string.h>

// Kernel vmap
vm_map_t kernel_vmap;

vm_mapping_t* _vm_mapping_insert_after(vm_mapping_t *mappings, vm_mapping_t *prev, vm_mapping_t *new_mapping) {
    vm_mapping_t *next = prev == NULL ? mappings : prev->next;

    if (prev == NULL) {
        new_mapping->next = new_mapping->prev = NULL;
        return new_mapping;
    } else {
        new_mapping->next = next;
        new_mapping->prev = prev;
        if (next != NULL) next->prev = new_mapping;
        prev->next = new_mapping;
        return mappings;
    }
}

vm_mapping_t* _vm_mapping_insert(vm_mapping_t *mappings, vm_mapping_t *new_mapping) {
    // Find position to insert
    vm_mapping_t *prev = NULL, *next = mappings;
    for (; next != NULL; prev = next, next = next->next) {
        if (new_mapping->vstart < next->vstart) break;
    }

    return _vm_mapping_insert_after(mappings, prev, new_mapping);
}

vm_mapping_t* _vm_mapping_remove(vm_mapping_t *mappings, vm_mapping_t *rm_mapping) {
    vm_mapping_t *prev = rm_mapping->prev, *next = rm_mapping->prev;
    if (prev != NULL) prev->next = next;
    if (next != NULL) next->prev = prev;

    // FIXME free mapping here
    return (prev == NULL ? next : mappings);
}

void _vm_mapping_remove_all(vm_mapping_t *mappings) {
    while (mappings != NULL) {
        mappings = _vm_mapping_remove(mappings, mappings);
    }
}

bool _vm_mapping_lookup(vm_mapping_t *mappings, vaddr_t vstart, vaddr_t vend, vm_mapping_t **lookup) {
    vm_mapping_t *prev = NULL, *next = mappings;
    for (; next != NULL; prev = next, next = next->next) {
        if (vstart < next->vstart) break;
    }

    if (prev != NULL && prev->vend > vstart) {
        *lookup = prev;
        return true;
    } else if (next != NULL && next->vstart < vend) {
        *lookup = next;
        return true;
    }

    *lookup = prev;
    return false;
}

void _vm_map_insert_mapping(vm_map_t *vmap, vm_mapping_t *prev, vaddr_t vstart, vaddr_t vend, vm_object_t *object, vm_offset_t offset, vm_prot_t prot) {
    size_t size = vend - vstart;

    // Insert a new mapping or check if we can merge this space into the previous mapping
    if (prev != NULL && prev->vend == vstart && prev->object == object && prev->prot == prot) {
        spinlock_writeacquire(&prev->object->lock);
        // Increase the size of the object
        size_t new_size = prev->offset + (prev->vend - prev->vstart) + size;
        if (new_size > prev->object->size) prev->object->size = new_size;
        spinlock_writerelease(&prev->object->lock);

        // Increase the size of the map and the end vaddr of the mapping
        vmap->size += size;
        prev->vend = vend;
    } else {
        // FIXME allocate and init entry
        vaddr_t kernel_virtual_start, kernel_virtual_end;
        vm_mapping_t *new_mapping = (vm_mapping_t*)pmap_steal_memory(sizeof(vm_mapping_t), &kernel_virtual_start, &kernel_virtual_end);

        new_mapping->vstart = vstart;
        new_mapping->vend = vend;
        new_mapping->prot = prot;
        new_mapping->object = object;
        new_mapping->offset = offset;

        atomic_inc(&object->refcnt);
        vmap->mappings = _vm_mapping_insert_after(vmap->mappings, prev, new_mapping);
        vmap->size += size;
    }
}

void vm_map_init(void) {
    // FIXME Init vm_map module
}

vm_map_t* vm_map_create(pmap_t *pmap, vaddr_t vmin, vaddr_t vmax) {
    // FIXME Allocate map
    vm_map_t *map;

    map->lock = SPINLOCK_INIT;
    map->mappings = NULL;
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
        _vm_mapping_remove_all(vmap->mappings);
    }
}

void vm_map_reference(vm_map_t *vmap) {
    atomic_inc(&vmap->refcnt);
}

kresult_t vm_map_enter_at(vm_map_t *vmap, vaddr_t vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot) {
    kassert(vmap != NULL);

    vm_mapping_t *prev;
    vaddr_t vstart = vaddr;
    vaddr_t vend = vstart + size;

    spinlock_writeacquire(&vmap->lock);

    // Make sure the address region specified isn't already mapped or partially mapped
    if (_vm_mapping_lookup(vmap->mappings, vaddr, vend, (vm_mapping_t**)&prev)) {
        spinlock_writerelease(&vmap->lock);
        return KRESULT_INVALID_ARGUMENT;
    }

    // Make sure it is within the total virtual address space
    if (vend > vmap->end) {
        spinlock_writerelease(&vmap->lock);
        return KRESULT_NO_SPACE;
    }

    _vm_map_insert_mapping(vmap, prev, vstart, vend, object, offset, prot);

    spinlock_writerelease(&vmap->lock);
    return KRESULT_OK;
}

kresult_t vm_map_enter(vm_map_t *vmap, vaddr_t *vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot) {
    kassert(vmap != NULL);

    // Assign the first virtual address to search with
    vaddr_t vstart = vmap->start;
    vaddr_t vend = vstart + size;

    spinlock_writeacquire(&vmap->lock);

    vm_mapping_t *prev = NULL, *next = vmap->mappings;
    for (; next != NULL; prev = next, next = next->next) {
        // Keep going until we find the place where this new mapping could be inserted
        if (vstart < next->vstart) continue;
        // Check the boundary conditions for overlap, if there is an overlap adjust the addresses and continue searching
        if (prev != NULL && prev->vend > vstart) {
            vstart = prev->vend;
            vend = prev->vend + size;
        } else if (next != NULL && next->vstart < vend) {
            vstart = next->vend;
            vend = next->vend + size;
        } else {
            break;
        }
    }

    // Found a space, check if is within the total virtual address space
    if (vend > vmap->end) {
        spinlock_writerelease(&vmap->lock);
        return KRESULT_NO_SPACE;
    }

    _vm_map_insert_mapping(vmap, prev, vstart, vend, object, offset, prot);

    spinlock_writerelease(&vmap->lock);
    *vaddr = vstart;
    return KRESULT_OK;
}

kresult_t vm_map_remove(vm_map_t *vmap, vaddr_t start, vaddr_t end) {
}

kresult_t vm_map_protect(vm_map_t *vmap, vaddr_t start, vaddr_t end, vm_prot_t new_prot) {
}

kresult_t vm_map_lookup(vm_map_t *vmap, vaddr_t vaddr, vm_prot_t fault_type, vm_object_t *object, vm_offset_t *offset, vm_prot_t *prot) {
}
