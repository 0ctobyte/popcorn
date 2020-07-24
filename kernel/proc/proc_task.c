/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kmem_slab.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/proc/proc_task.h>

proc_task_t kernel_task;
proc_task_t *current_task;
proc_task_t proc_task_template;

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

    spinlock_init(&proc_task_list.lock);
    list_init(&proc_task_list.tasks);

    // Initialize the kernel task
    spinlock_init(&kernel_task.lock);
    kernel_task.pid = 0;
    kernel_task.refcnt = 1;
    kernel_task.state = PROC_TASK_STATE_ACTIVE;
    kernel_task.suspend_cnt = 0;
    kernel_task.vm_map = vm_map_kernel();
    list_init(&kernel_task.ll_threads);
    kernel_task.num_threads = 0;
    kernel_task.parent = NULL;
    list_init(&kernel_task.ll_children);
    list_node_init(&kernel_task.ll_snode);
    list_node_init(&kernel_task.ll_tnode);

    spinlock_acquire_irq(&proc_task_list.lock);
    list_insert_last(&proc_task_list.tasks, &kernel_task.ll_tnode);
    spinlock_release_irq(&proc_task_list.lock);

    spinlock_init(&proc_task_template.lock);
    proc_task_template.pid = 0;
    proc_task_template.refcnt = 0;
    proc_task_template.state = PROC_TASK_STATE_NEW;
    proc_task_template.suspend_cnt = 0;
    proc_task_template.vm_map = NULL;
    list_init(&proc_task_template.ll_threads);
    proc_task_template.num_threads = 0;
    proc_task_template.parent = proc_task_kernel();
    list_init(&proc_task_template.ll_children);
    list_node_init(&proc_task_template.ll_snode);
    list_node_init(&proc_task_template.ll_tnode);
}
