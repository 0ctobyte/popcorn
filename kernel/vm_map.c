#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kassert.h>
#include <string.h>

// Kernel vmap
vm_map_t kernel_vmap;

rbtree_compare_result_t _vm_mapping_compare(rbtree_node_t *n1, rbtree_node_t *n2) {
    vm_mapping_t *m1 = rbtree_entry(n1, vm_mapping_t, rb_node), *m2 = rbtree_entry(n2, vm_mapping_t, rb_node);
    return (m1->vstart >= m2->vend) ? RBTREE_COMPARE_GT : (m1->vend <= m2->vstart) ? RBTREE_COMPARE_LT : RBTREE_COMPARE_EQ;
}

void _vm_mapping_destroy(rbtree_node_t *node) {
    // FIXME free mapping here
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_node);
}

void vm_map_init(void) {
    // FIXME Init vm_map module
}

vm_map_t* vm_map_create(pmap_t *pmap, vaddr_t vmin, vaddr_t vmax) {
    // FIXME Allocate map
    vm_map_t *map;

    map->lock = SPINLOCK_INIT;
    map->rb_mappings = RBTREE_INITIALIZER;
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
        rbtree_clear(&vmap->rb_mappings, _vm_mapping_destroy);
    }
}

void vm_map_reference(vm_map_t *vmap) {
    atomic_inc(&vmap->refcnt);
}

kresult_t vm_map_enter_at(vm_map_t *vmap, vaddr_t vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot) {
    kassert(vmap != NULL);

    rbtree_slot_t slot;
    rbtree_node_t *predecessor_node;
    vm_mapping_t tmp = (vm_mapping_t){ .rb_node = RBTREE_NODE_INITIALIZER, .vstart = vaddr, .vend = vaddr + size, .prot = prot, .object = object, .offset = offset };

    // Make sure it is within the total virtual address space
    if (tmp.vend > vmap->end) {
        return KRESULT_NO_SPACE;
    }

    spinlock_writeacquire(&vmap->lock);

    // Make sure the address region specified isn't already mapped or partially mapped
    if (rbtree_search_predecessor(&vmap->rb_mappings, _vm_mapping_compare, &tmp.rb_node, &predecessor_node, &slot)) {
        spinlock_writerelease(&vmap->lock);
        return KRESULT_INVALID_ARGUMENT;
    }

    vm_mapping_t *predecessor = rbtree_entry(predecessor_node, vm_mapping_t, rb_node);

    // Check if we can merge the predecessor mapping with this new mapping
    if (predecessor != NULL && predecessor->vend == tmp.vstart && predecessor->object == tmp.object && predecessor->prot == tmp.prot) {
        // Increase the size of the object
        spinlock_writeacquire(&predecessor->object->lock);
        size_t new_size = predecessor->offset + (predecessor->vend - predecessor->vstart) + size;
        if (new_size > predecessor->object->size) predecessor->object->size = new_size;
        spinlock_writerelease(&predecessor->object->lock);

        // Increase the size of the map and the end vaddr of the mapping
        vmap->size += size;
        predecessor->vend = tmp.vend;
    } else {
        // FIXME allocate and init entry
        vaddr_t kernel_virtual_start, kernel_virtual_end;
        vm_mapping_t *new_mapping = (vm_mapping_t*)pmap_steal_memory(sizeof(vm_mapping_t), &kernel_virtual_start, &kernel_virtual_end);
        memcpy(new_mapping, &tmp, sizeof(vm_mapping_t));

        // Insert the new mapping
        kassert(rbtree_insert_slot(&vmap->rb_mappings, slot, &new_mapping->rb_node));
        vmap->size += size;
        atomic_inc(&object->refcnt);
    }

    spinlock_writerelease(&vmap->lock);
    return KRESULT_OK;
}

kresult_t vm_map_enter(vm_map_t *vmap, vaddr_t *vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot) {
    kassert(vmap != NULL);

    rbtree_slot_t slot;
    rbtree_node_t *predecessor_node;
    vm_mapping_t tmp = (vm_mapping_t){ .rb_node = RBTREE_NODE_INITIALIZER, .vstart = 0, .vend = size, .prot = prot, .object = object, .offset = offset };

    spinlock_writeacquire(&vmap->lock);

    // Have we found an open virtual address range that fits the specified size?
    if (tmp.vstart == 0) {
        spinlock_writerelease(&vmap->lock);
        return KRESULT_NO_SPACE;
    }

    spinlock_writerelease(&vmap->lock);

    *vaddr = tmp.vstart;
    return KRESULT_OK;
}

kresult_t vm_map_remove(vm_map_t *vmap, vaddr_t start, vaddr_t end) {
}

kresult_t vm_map_protect(vm_map_t *vmap, vaddr_t start, vaddr_t end, vm_prot_t new_prot) {
}

kresult_t vm_map_lookup(vm_map_t *vmap, vaddr_t vaddr, vm_prot_t fault_type, vm_object_t *object, vm_offset_t *offset, vm_prot_t *prot) {
}
