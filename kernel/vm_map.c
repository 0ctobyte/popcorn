#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kmem.h>
#include <kernel/kassert.h>
#include <kernel/slab.h>
#include <lib/asm.h>

// Kernel vmap
vm_map_t kernel_vmap;

// Slab for kernel vm_mapping_t struct
#define NUM_KERNEL_VM_MAPPING_STRUCTS (64)
slab_t kernel_vm_mapping_slab;
slab_buf_t kernel_vm_mapping_slab_buf;

rbtree_compare_result_t _vm_mapping_compare(rbtree_node_t *n1, rbtree_node_t *n2) {
    vm_mapping_t *m1 = rbtree_entry(n1, vm_mapping_t, rb_node), *m2 = rbtree_entry(n2, vm_mapping_t, rb_node);
    return (m1->vstart >= m2->vend) ? RBTREE_COMPARE_GT : (m1->vend <= m2->vstart) ? RBTREE_COMPARE_LT : RBTREE_COMPARE_EQ;
}

rbtree_compare_result_t _vm_mapping_compare_hole(rbtree_node_t *n1, rbtree_node_t *n2) {
    vm_mapping_t *m1 = rbtree_entry(n1, vm_mapping_t, rb_node), *m2 = rbtree_entry(n2, vm_mapping_t, rb_node);
    return (m1->hole_size > m2->vend) ? RBTREE_COMPARE_GT : RBTREE_COMPARE_LT;
}

void _vm_mapping_hole_update(vm_map_t *vmap, vm_mapping_t *mapping, size_t new_hole_size) {
    mapping->hole_size = new_hole_size;
    kassert(rbtree_remove(&vmap->rb_holes, &mapping->rb_hole));
    if (new_hole_size > 0 ) kassert(rbtree_insert(&vmap->rb_holes, _vm_mapping_compare_hole, &mapping->rb_hole));
}

void _vm_mapping_hole_insert(vm_map_t *vmap, vm_mapping_t *predecessor, vm_mapping_t *new_mapping) {
    kassert(new_mapping != NULL);

    // Update the hole size for the predecessor
    if (predecessor != NULL) _vm_mapping_hole_update(vmap, predecessor, new_mapping->vstart - predecessor->vend);

    // Assumption is that this function is called after the new mapping has been inserted into the mapping tree and list
    // So we should be able to get the successor if it exists
    vm_mapping_t *successor = list_entry(list_next(&new_mapping->ll_node), vm_mapping_t, ll_node);
    size_t new_mapping_hole_size = ((successor != NULL) ? successor->vstart : vmap->end) - new_mapping->vend;

    // Insert the new mapping into the hole tree
    new_mapping->hole_size = new_mapping_hole_size;
    if (new_mapping_hole_size > 0) kassert(rbtree_insert(&vmap->rb_holes, _vm_mapping_compare_hole, &new_mapping->rb_hole));
}

void _vm_mapping_insert(vm_map_t *vmap, rbtree_slot_t slot, vm_mapping_t *predecessor, vm_mapping_t *new_mapping) {
    kassert(rbtree_insert_slot(&vmap->rb_mappings, slot, &new_mapping->rb_node));
    kassert(list_insert_after(&vmap->ll_mappings, &predecessor->ll_node, &new_mapping->ll_node));
}

void _vm_mapping_destroy(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_node);
    kmem_free(mapping, sizeof(vm_mapping_t));
}

vm_mapping_t* _vm_mapping_alloc(vm_map_t *vmap) {
    vm_mapping_t *mapping;

    if (vm_map_kernel() == vmap) {
        // Use the slab instead of kmem for kernel vm_mapping_t structs
        // If we're out of buffers from the slab then the kernel is in an unrecoverable state
        mapping = (vm_mapping_t*)slab_alloc(&kernel_vm_mapping_slab);
        kassert(mapping != NULL);
    } else {
        mapping = (vm_mapping_t*)kmem_alloc(sizeof(vm_mapping_t));
    }

    return mapping;
}

void vm_map_init(void) {
    // Initialize the slab to allocate kernel vm_mapping_t structs from. We can't use the general
    // purpose allocators because of circular dependencies if we are out of kernel virtual memory
    size_t size = NUM_KERNEL_VM_MAPPING_STRUCTS * sizeof(vm_mapping_t);
    void *buf = (void*)pmap_steal_memory(size, NULL, NULL);
    slab_init(&kernel_vm_mapping_slab, &kernel_vm_mapping_slab_buf, buf, size, sizeof(vm_mapping_t));
}

vm_map_t* vm_map_create(pmap_t *pmap, vaddr_t vmin, vaddr_t vmax) {
    vm_map_t *map = (vm_map_t*)kmem_alloc(sizeof(vm_map_t));

    *map = (vm_map_t){ .lock = SPINLOCK_INIT, .rb_mappings = RBTREE_INITIALIZER, .rb_holes = RBTREE_INITIALIZER,
        .pmap = pmap, .start = vmin, .end = vmax, .size = 0, .refcnt = 0 };

    vm_map_reference(map);
    return map;
}

void vm_map_destroy(vm_map_t *vmap) {
    atomic_dec(&vmap->refcnt);

    if (vmap->refcnt == 0) {
        rbtree_clear(&vmap->rb_mappings, _vm_mapping_destroy);
        kmem_free(vmap, sizeof(vm_map_t));
    }
}

void vm_map_reference(vm_map_t *vmap) {
    atomic_inc(&vmap->refcnt);
}

kresult_t vm_map_enter_at(vm_map_t *vmap, vaddr_t vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot) {
    kassert(vmap != NULL);

    rbtree_slot_t slot = 0;
    rbtree_node_t *predecessor_node = NULL;
    vm_mapping_t tmp = (vm_mapping_t){ .rb_node = RBTREE_NODE_INITIALIZER, .rb_hole = RBTREE_NODE_INITIALIZER, .hole_size = 0,
        .vstart = vaddr, .vend = vaddr + size, .prot = prot, .object = object, .offset = offset };

    // Make sure it is within the total virtual address space
    if (tmp.vstart < vmap->start && tmp.vend > vmap->end) {
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

        // Update the predecessor's hole size and hole tree
        _vm_mapping_hole_update(vmap, predecessor, predecessor->hole_size -(tmp.vend - tmp.vstart));
    } else {
        vm_mapping_t *new_mapping = _vm_mapping_alloc(vmap);
        _fast_move((uintptr_t)new_mapping, (uintptr_t)&tmp, sizeof(vm_mapping_t));

        // Insert the new mapping
        _vm_mapping_insert(vmap, slot, predecessor, new_mapping);

        // Update the hole tree
        _vm_mapping_hole_insert(vmap, predecessor, new_mapping);

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

    *vaddr = tmp.vstart;
    return KRESULT_OK;
}

kresult_t vm_map_remove(vm_map_t *vmap, vaddr_t start, vaddr_t end) {
}

kresult_t vm_map_protect(vm_map_t *vmap, vaddr_t start, vaddr_t end, vm_prot_t new_prot) {
}

kresult_t vm_map_lookup(vm_map_t *vmap, vaddr_t vaddr, vm_prot_t fault_type, vm_object_t *object, vm_offset_t *offset, vm_prot_t *prot) {
}
