/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _PROC_SCHEDULER_H_
#define _PROC_SCHEDULER_H_

#include <kernel/rbtree.h>
#include <kernel/proc/proc_types.h>

struct proc_thread_s;

typedef struct proc_scheduler_context_s {
    rbtree_node_t rb_node;       // Red/black tree linkage
    unsigned long vruntime;      // Thread's CPU runtime
} proc_scheduler_context_t;

// Initializes the scheduler data structures
void proc_scheduler_init(void);

// Add a new thread or newly awoken thread to the scheduler
void proc_scheduler_add(struct proc_thread_s *thread);

// Just removes the thread from the scheduler
void proc_scheduler_remove(struct proc_thread_s *thread);

// Chooses next thread to run, may switch to another more deserving thread
void proc_scheduler_choose(void);

// Lets the scheduler prepare for a thread to be suspended or put to sleep
// This will cause a context switch to another runnable thread
void proc_scheduler_sleep(void);

// Macro to compare vruntime of two threads to determine which is more deserving of CPU time
// True if t1 deserves time over t2, false otherwise
#define proc_scheduler_deserve(t1, t2) ((t1)->sched.vruntime < (t2)->sched.vruntime)

#endif // _PROC_SCHEDULER_H_
