#include <kernel/kmem_slab.h>
#include <kernel/vm/vm_types.h>
#include <kernel/proc/proc_task.h>
#include <kernel/proc/proc_thread.h>

size_t kernel_stack_size;
proc_thread_t *current_thread;

// proc_thread_t slab
#define PROC_THREAD_SLAB_NUM (1024)
kmem_slab_t proc_thread_slab;

// Kernel stack slab
#define KERNEL_STACK_SLAB_NUM (1024)
kmem_slab_t kernel_stack_slab;

void proc_thread_init(void) {
    // Initialize the slab for allocating proc_thread_t structs
    kmem_slab_create(&proc_thread_slab, sizeof(proc_thread_t), PROC_THREAD_SLAB_NUM);

    // Initialize the slab for allocating kernel stacks. Kernel stacks are the size of one page
    kernel_stack_size = PAGESIZE;
    kmem_slab_create(&kernel_stack_slab, kernel_stack_size, KERNEL_STACK_SLAB_NUM);
}
