#include <kernel/kmem_slab.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/proc/proc_task.h>

proc_task_t kernel_task;
proc_task_t *current_task;

// proc_task_t slab
#define PROC_TASK_SLAB_NUM (256)
kmem_slab_t proc_task_slab;

// List of all processes
typedef struct {
    spinlock_t lock; // List lock
    list_t tasks;    // List of all tasks
} proc_task_list_t;

proc_task_list_t proc_task_list;

void proc_task_init(void) {
    // Initialize the slab for allocating proc_task_t structs
    kmem_slab_create(&proc_task_slab, sizeof(proc_task_t), PROC_TASK_SLAB_NUM);

    proc_task_list.lock = SPINLOCK_INIT;
    proc_task_list.tasks = LIST_INITIALIZER;

    // Initialize the kernel task
    kernel_task.lock = SPINLOCK_INIT;
    kernel_task.pid = 0;
    kernel_task.refcnt = 1;
    kernel_task.state = PROC_TASK_STATE_ACTIVE;
    kernel_task.vm_map = vm_map_kernel();
    kernel_task.ll_threads = LIST_INITIALIZER;
    kernel_task.num_threads = 0;
    kernel_task.priority = UINT8_MAX;
    kernel_task.parent = NULL;
    kernel_task.ll_children = LIST_INITIALIZER;
    kernel_task.ll_snode = LIST_NODE_INITIALIZER;
    kernel_task.ll_tnode = LIST_NODE_INITIALIZER;
    kernel_task.suspend_cnt = 0;

    spinlock_acquire(&proc_task_list.lock);
    list_insert_last(&proc_task_list.tasks, &kernel_task.ll_tnode);
    spinlock_release(&proc_task_list.lock);
}
