#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/devicetree.h>
#include <kernel/rbtree.h>
#include <kernel/kmem.h>
#include <kernel/arch/asm.h>
#include <kernel/arch/exceptions.h>
#include <kernel/arch/interrupts.h>
#include <kernel/vm/vm_init.h>
#include <kernel/vm/vm_map.h>
#include <kernel/vm/vm_object.h>
#include <kernel/vm/vm_km.h>
#include <kernel/vfs/vfs_mount.h>
#include <kernel/vfs/vfs_node.h>

extern paddr_t kernel_physical_start;
extern paddr_t kernel_physical_end;
extern void _relocate(unsigned long dst, unsigned long src, size_t size);

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;

void vm_mapping_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_node);
    kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n", mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
}

void _vm_mapping_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_node);
    kprintf("%x:%x:%c\n", (vaddr_t)node, mapping->vstart, rbtree_is_black(node) ? 'b' : 'r');
}

void vm_mapping_hole_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_hole);
    kprintf("mapping hole - vstart = %p, vend = %p\n", mapping->vend, mapping->vend + mapping->hole_size);
}

void _vm_mapping_hole_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_hole);
    kprintf("%x:%x:%c\n", (vaddr_t)mapping, mapping->vstart, rbtree_is_black(node) ? 'b' : 'r');
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

    // Relocate the kernel to the base of memory
    if (kernel_physical_start != MEMBASEADDR) {
        size_t kernel_size = kernel_physical_end - kernel_physical_start;
        _relocate(MEMBASEADDR, kernel_physical_start, kernel_size);
        kernel_physical_start = MEMBASEADDR;
        kernel_physical_end = kernel_physical_start + kernel_size;
    }

    kprintf("Hello World\n");

    vm_init();

    kprintf("vm_init() - done!\n");

    kprintf("sizeof(vfs_node_t) == %llu, sizeof(vfs_mount_t) == %llu\n", sizeof(vfs_node_t), sizeof(vfs_mount_t));
    kmem_stats();

    // kprintf("vmap - start = %p, end = %p\n", vm_map_kernel()->start, vm_map_kernel()->end);

    // vaddr_t kernel_virtual_start, kernel_virtual_end;
    // kresult_t res;

    // pmap_virtual_space(&kernel_virtual_start, &kernel_virtual_end);
    // print_mappings();

    // size_t size = PAGESIZE;
    // #define NUM_ALLOCS 50
    // void *buf[NUM_ALLOCS];

    // for (int i = 0; i < NUM_ALLOCS; i++) {
    //     buf[i] = kmem_alloc(size);
    //     kmem_stats();
    // }

    // for (int i = NUM_ALLOCS-1; i >= 0; i--) {
    //     kmem_free(buf[i], size);
    //     kmem_stats();
    // }

    // print_mappings();
}
