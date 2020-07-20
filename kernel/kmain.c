#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/fdt.h>
#include <kernel/devicetree.h>
#include <kernel/rbtree.h>
#include <kernel/console.h>
#include <kernel/irq.h>
#include <kernel/arch/arch_exceptions.h>
#include <kernel/arch/arch_timer.h>
#include <kernel/vm/vm_types.h>
#include <kernel/vm/vm_init.h>
#include <kernel/vm/vm_map.h>
#include <kernel/vm/vm_object.h>
#include <kernel/vm/vm_km.h>
#include <kernel/proc/proc_init.h>
#include <kernel/proc/proc_task.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/vfs/vfs_mount.h>
#include <kernel/vfs/vfs_node.h>
#include <platform/platform.h>

extern fdt_header_t *fdt_header;

extern void _relocate(unsigned long dst, unsigned long src, size_t size);

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;

void vm_mapping_walk(rbtree_node_t *node) {
    vm_mapping_t *mapping = rbtree_entry(node, vm_mapping_t, rb_snode);
    kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n",
        mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
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
        kprintf("mapping - vstart = %p, vend = %p, prot = %p, object = %p, offset = %p\n",
            mapping->vstart, mapping->vend, mapping->prot, mapping->object, mapping->offset);
    }
}

void thread_start(void) {
    proc_thread_t *thread = list_entry(list_first(&proc_task_kernel()->ll_threads), proc_thread_t, ll_tnode);
    for (;;) {
        kprintf("%f s: thread id = %d\n", arch_timer_get_secs(), proc_thread_current()->tid);
        proc_thread_switch(thread);
    }
}

void kmain(void) {
    // Setup the exception vector table
    arch_exceptions_init();

    // Need the FDT header offset from the beginning of the kernel image in order to adjust the pointer later
    // when the VM is initialized
    uintptr_t fdth_offset = (uintptr_t)fdt_header - kernel_physical_start;
    if (!devicetree_find_memory(&MEMBASEADDR, &MEMSIZE)) HALT();

    // Relocate the kernel to the base of memory
    if (kernel_physical_start != MEMBASEADDR) {
        size_t kernel_size = kernel_physical_end - kernel_physical_start;
        _relocate(MEMBASEADDR, kernel_physical_start, kernel_size);
        kernel_physical_start = MEMBASEADDR;
        kernel_physical_end = kernel_physical_start + kernel_size;
        fdt_header = (fdt_header_t*)(kernel_physical_start + fdth_offset);
    }

    vm_init();

    // Update FDT header virtual address
    fdt_header = (fdt_header_t*)(kernel_virtual_start + fdth_offset);

    platform_init(fdt_header);
    console_init();

    kprintf("console_init() - done!\n");
    kprintf("platform_init() - done!\n");

    kprintf("vm_init() - done!\n");

    //fdt_dump(fdt_header);

    proc_init();
    kprintf("proc_init() - done!\n");

    irq_init();
    kprintf("irq_init() - done!\n");

    arch_timer_start_secs(1);

    proc_thread_t *thread;
    kassert(proc_thread_create(proc_task_kernel(), &thread) == KRESULT_OK);
    proc_thread_set_entry(thread, thread_start);
    proc_thread_resume(thread);

    //for (;;) {
    //    kprintf("%f ms: thread id = %d\n", arch_timer_get_msecs(), proc_thread_current()->tid);
    //    proc_thread_switch(thread);
    //}

    kprintf("sizeof(vm_page_t) == %llu\n", sizeof(vm_page_t));

    // print_mappings();
}
