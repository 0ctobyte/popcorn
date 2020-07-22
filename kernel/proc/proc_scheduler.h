#ifndef _PROC_SCHEDULER_H_
#define _PROC_SCHEDULER_H_

#include <kernel/proc/proc_types.h>

struct proc_thread_s;

typedef struct proc_scheduler_context_s {
    unsigned long vruntime;      // Thread's CPU runtime
    unsigned long exec_start;    // The time when the thread was scheduled for execution
} proc_scheduler_context_t;

// Initializes the scheduler data structures
void proc_scheduler_init(void);

// Initializes a thread's scheduler related data structure
void proc_scheduler_thread_init(struct proc_thread_s *thread);

// Lets the scheduler prepare for a thread to be put to sleep
void proc_scheduler_thread_sleep(struct proc_thread_s *thread);

// Allows the scheduler to update a thread's scheduling parameters after it has awoken (i.e. virtual runtime)
void proc_scheduler_thread_wake(struct proc_thread_s *thread);

// Called when a thread yields, may switch to another more deserving thread
void proc_scheduler_yield(void);

// Chooses next thread to run, may switch to another more deserving thread
void proc_scheduler_choose(void);

// Macro to compare vruntime of two threads to determine which is more deserving of CPU time
// True if t1 deserves time over t2, false otherwise
#define proc_scheduler_deserve(t1, t2) ((t1)->sched.vruntime < (t2)->sched.vruntime)

#endif // _PROC_SCHEDULER_H_
