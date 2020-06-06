#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/exceptions.h>
#include <kernel/interrupts.h>
#include <kernel/devicetree.h>
#include <kernel/vmm.h>
#include <kernel/vm_map.h>
#include <kernel/vm_object.h>
#include <kernel/vm_km.h>

#include <lib/asm.h>
#include <lib/rbtree.h>

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;

void vm_mapping_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_node);
    kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n", mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
}

void vm_mapping_hole_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_hole);
    kprintf("mapping hole - vstart = %p, vend = %p\n", mapping->vend, mapping->vend + mapping->hole_size);
}

void print_mappings(void) {
    rbtree_walk_inorder(&vm_map_kernel()->rb_mappings, vm_mapping_walk);
    rbtree_walk_inorder(&vm_map_kernel()->rb_holes, vm_mapping_hole_walk);

    vm_mapping_t *mapping = NULL;
    list_for_each_entry(&vm_map_kernel()->ll_mappings, mapping, ll_node) {
        kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n", mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
    }
}

void kmain(void) {
    // Setup the exception vector table
    exceptions_init();

    if (!devicetree_find_memory(&MEMBASEADDR, &MEMSIZE)) HALT();

    kprintf("Hello World\n");

    vmm_init();

    kprintf("vmm_init() - done!\n");

    kprintf("vmap - start = %p, end = %p\n", vm_map_kernel()->start, vm_map_kernel()->end);

    vaddr_t kernel_virtual_start, kernel_virtual_end;
    kresult_t res;

    pmap_virtual_space(&kernel_virtual_start, &kernel_virtual_end);

    res = vm_map_enter_at(vm_map_kernel(), vm_map_kernel()->end - PAGESIZE, PAGESIZE, &kernel_object, 0, VM_PROT_ALL);
    kassert(res == KRESULT_OK);

    print_mappings();
}
