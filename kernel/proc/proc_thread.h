#ifndef _PROC_THREAD_H_
#define _PROC_THREAD_H_

#include <kernel/spinlock.h>
#include <kernel/list.h>
#include <kernel/kresult.h>
#include <kernel/arch/arch_thread.h>
#include <kernel/vm/vm_types.h>
#include <kernel/proc/proc_types.h>
#include <kernel/proc/proc_task.h>

typedef enum {
    PROC_THREAD_STATE_SUSPENDED,
    PROC_THREAD_STATE_RUNNABLE,
    PROC_THREAD_STATE_RUNNING,
    PROC_THREAD_STATE_SLEEPING,
    PROC_THREAD_STATE_TERMINATED,
} proc_thread_state_t;

typedef struct proc_thread_s {
    spinlock_t lock;                // Interrupt disabling spin lock
    proc_thread_id_t tid;           // Thread ID
    proc_thread_state_t state;      // Thread state
    bool suspend_requested;         // Indicates if a suspend was requested
    proc_priority_t priority;       // Thread priority
    proc_priority_t sched_priority; // Priority computed by scheduler
    unsigned int refcnt;            // Reference count
    proc_task_t *task;              // Task this thread belongs to
    proc_event_t event;             // Event this thread is sleeping on
    list_node_t ll_enode;           // Event hash table bucket list linkage
    list_node_t ll_tnode;           // Thread list linkage
    list_node_t ll_qnode;           // Run queue list linkage
    vaddr_t kernel_stack;           // Kernel stack VA
    arch_thread_context_t context;  // User saved context
} proc_thread_t;

extern size_t kernel_stack_size;
extern proc_thread_t *current_thread;

// Initializes the proc_thread module
void proc_thread_init(void);

// Creates a new thread in the given task. The new thread is given in thread
kresult_t proc_thread_create(proc_task_t *task, proc_thread_t **thread);

// Increments the reference counter on the thread
void proc_thread_reference(proc_thread_t *thread);

// Decrements the reference counter on the thread. If zero, terminates the thread
void proc_thread_unreference(proc_thread_t *thread);

// Terminates the thread
kresult_t proc_thread_terminate(proc_thread_t *thread);

// Resume the thread
kresult_t proc_thread_resume(proc_thread_t *thread);

// Suspends the thread
kresult_t proc_thread_suspend(proc_thread_t *thread);

// Puts the thread to sleep. Interruptible indicates whether the sleep can be interrupted
// by something other then the sleep event
void proc_thread_sleep(proc_thread_t *thread, proc_event_t event, bool interruptible);

// Wakes the thread clearing whatever event it was sleeping on
void proc_thread_wake(proc_thread_t *thread);

// Do whatever necessary to run the given thread
void proc_thread_run(proc_thread_t *thread);

// Switches the current running thread to the given new thread
void proc_thread_switch(proc_thread_t *new_thread);

// Sets the state of the thread
kresult_t proc_thread_set_context(proc_thread_t *thread, arch_thread_context_t new_context);

// Gets the current state of the thread. Returns the result in state
kresult_t proc_thread_get_context(proc_thread_t *thread, arch_thread_context_t *context);

// Sets the user space entry point for the given thread
kresult_t proc_thread_set_entry(proc_thread_t *thread, void *entry_point_addr);

// Sets the user space stack pointer for the given thread
kresult_t proc_thread_set_stack(proc_thread_t *thread, void *stack_addr);

// Get the current thread
#define proc_thread_current() (current_thread)

#endif // _PROC_THREAD_H_
