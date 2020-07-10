#include <kernel/kassert.h>
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

    // Create a thread for the currently running kernel code
    proc_thread_t *kernel_thread = kmem_slab_alloc(&proc_thread_slab);
    kassert(kernel_thread != NULL);

    kernel_thread->lock = SPINLOCK_INIT;
    kernel_thread->tid = 0;
    kernel_thread->task = proc_task_kernel();
    kernel_thread->state = PROC_THREAD_STATE_RUNNING;
    kernel_thread->priority = proc_task_kernel()->priority;
    kernel_thread->refcnt = 1;
    kernel_thread->event = 0;
    kernel_thread->ll_enode = LIST_NODE_INITIALIZER;
    kernel_thread->ll_tnode = LIST_NODE_INITIALIZER;
    kernel_thread->ll_qnode = LIST_NODE_INITIALIZER;
    int stack_var;
    kernel_thread->kernel_stack = ROUND_PAGE_DOWN(&stack_var);

    spinlock_acquire(&proc_task_kernel()->lock);
    kassert(list_insert_last(&proc_task_kernel()->ll_threads, &kernel_thread->ll_tnode));
    proc_task_kernel()->num_threads++;
    spinlock_release(&proc_task_kernel()->lock);

    proc_thread_current() = kernel_thread;
}

void proc_thread_switch(proc_thread_t *new_thread) {
    kassert(new_thread != NULL);

    if (proc_thread_current() == new_thread) {
        spinlock_acquire(&new_thread->lock);
    } else if (proc_thread_current() < new_thread) {
        spinlock_acquire(&proc_thread_current()->lock);
        spinlock_acquire(&new_thread->lock);
    } else {
        spinlock_acquire(&new_thread->lock);
        spinlock_acquire(&proc_thread_current()->lock);
    }

    proc_thread_t *cur_thread = proc_thread_current();
    new_thread = arch_thread_switch(new_thread, cur_thread);
    proc_thread_current() = cur_thread;

    if (cur_thread == new_thread) {
        spinlock_release(&new_thread->lock);
    } else if (cur_thread < new_thread) {
        spinlock_release(&cur_thread->lock);
        spinlock_release(&new_thread->lock);
    } else {
        spinlock_release(&new_thread->lock);
        spinlock_release(&cur_thread->lock);
    }
}
