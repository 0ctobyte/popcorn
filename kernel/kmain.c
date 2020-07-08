#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/devicetree.h>
#include <kernel/rbtree.h>
#include <kernel/arch/arch_exceptions.h>
#include <kernel/vm/vm_init.h>
#include <kernel/vm/vm_map.h>
#include <kernel/vm/vm_object.h>
#include <kernel/vm/vm_km.h>
#include <kernel/proc/proc_init.h>
#include <kernel/vfs/vfs_mount.h>
#include <kernel/vfs/vfs_node.h>

extern paddr_t kernel_physical_start;
extern paddr_t kernel_physical_end;
extern void _relocate(unsigned long dst, unsigned long src, size_t size);

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;

void vm_mapping_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_snode);
    kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n", mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
}

void _vm_mapping_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_snode);
    kprintf("%x:%x:%c\n", (vaddr_t)node, mapping->vstart, rbtree_is_black(node) ? 'b' : 'r');
}

void vm_mapping_hole_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_hnode);
    kprintf("mapping hole - vstart = %p, vend = %p\n", mapping->vend, mapping->vend + mapping->hole_size);
}

void _vm_mapping_hole_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_hnode);
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
    arch_exceptions_init();

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

    // FIXME switching UART to VA address
    extern unsigned long uart_base_addr;
    vaddr_t uart_base_va = 0xFFFFFFFFC0000000;
    pmap_kenter_pa(uart_base_va, uart_base_addr & ~(PAGESIZE - 1), VM_PROT_DEFAULT, PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    uart_base_addr = uart_base_va + (uart_base_addr & (PAGESIZE - 1));

    kprintf("vm_init() - done!\n");

    proc_init();

    kprintf("proc_init() - done!\n");

    kprintf("sizeof(vm_page_t) == %llu\n", sizeof(vm_page_t));

    print_mappings();
}
