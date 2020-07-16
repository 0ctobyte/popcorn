#ifndef _PROC_SCHEDULER_H_
#define _PROC_SCHEDULER_H_

#include <kernel/proc/proc_types.h>

struct proc_thread_s;

typedef struct proc_scheduler_context_s {
    proc_priority_t priority;           // Thread's fixed priority
    proc_priority_t inherited_priority; // Thread's inherited priority
    unsigned long virtual_runtime;      // Thread's CPU runtime
} proc_scheduler_context_t;

// Initializes the scheduler data structures
void proc_scheduler_init(void);

// Initializes a thread's scheduler related data structure
void proc_scheduler_thread_init(struct proc_thread_s *thread);

// Updates virtual_runtime for thread that has just awakened
void proc_scheduler_thread_wake(struct proc_thread_s *thread);

// Called when a thread yields, may switch to another more worthy thread
void proc_scheduler_yield(void);

// Chooses next thread to run, may switch to another more worthy thread
void proc_scheduler_choose(void);

#endif // _PROC_SCHEDULER_H_
