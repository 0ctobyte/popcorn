#ifndef _PROC_TASK_H_
#define _PROC_TASK_H_

#include <kernel/spinlock.h>
#include <kernel/list.h>
#include <kernel/kresult.h>
#include <kernel/vm/vm_map.h>
#include <kernel/proc/proc_types.h>

typedef enum {
    PROC_TASK_STATE_NEW,
    PROC_TASK_STATE_ACTIVE,
    PROC_TASK_STATE_SUSPENDED,
    PROC_TASK_STATE_TERMINATED,
} proc_task_state_t;

typedef struct proc_task_s {
    spinlock_t lock;            // Interrupt disabling spin lock
    proc_id_t pid;              // Task/process ID
    unsigned int refcnt;        // Reference count
    proc_task_state_t state;    // Task state
    vm_map_t *vm_map;           // Virtual memory map
    list_t ll_threads;          // All threads belonging to this task
    size_t num_threads;         // Number of threads in task
    proc_priority_t priority;   // Default priority passed on to new threads when created
    struct proc_task_s *parent; // Parent task
    list_t ll_children;         // List of all children tasks
    list_node_t ll_snode;       // Sibling task list linkage
    list_node_t ll_tnode;       // List linkage for list of all tasks
    unsigned int suspend_cnt;   // Number of outstanding suspends
} proc_task_t;

extern proc_task_t kernel_task;

// Initializes the kernel task
void proc_task_init(void);

// Create a new task that is the child of the given parent. The inherit flag creates a copy of the vm_map for the new task
// The new task is returned in child
kresult_t proc_task_create(proc_task_t *parent, bool inherit, proc_task_t **child);

// Increments the reference count on the task
void proc_task_reference(proc_task_t *task);

// Decrements the reference count on the task. If the count reaches zero the task will be terminated
void proc_task_unreference(proc_task_t *task);

// Terminates the task and all it's threads
kresult_t proc_task_terminate(proc_task_t *task);

// Resumes the task and all it's threads
kresult_t proc_task_resume(proc_task_t *task);

// Increments the suspend_cnt; suspends the task and all it's threads if suspend_cnt == 1
kresult_t proc_task_suspend(proc_task_t *task);

// Get the current task
#define proc_task_current() (proc_thread_current()->task)

// Get the kernel task
#define proc_task_kernel()  (&kernel_task)

#endif // _PROC_TASK_H_
