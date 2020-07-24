/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/rbtree.h>
#include <kernel/spinlock.h>
#include <kernel/arch/arch_timer.h>
#include <kernel/proc/proc_task.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/proc/proc_scheduler.h>

#define PROC_SCHEDULER_QUANTUM_US (10000)

typedef struct {
    spinlock_t lock;            // Simple non-preemptive lock
    rbtree_t rb_threads;        // Thread tree keyed of virtual_runtime
    size_t num_threads;         // Number of threads currently in the tree
    unsigned long min_vruntime; // Track the current smallest vruntime in the tree
    unsigned long exec_start;   // The time when a thread was scheduled for execution
    unsigned long quantum;      // How long to run the current thread before deciding to switch threads
} proc_scheduler_t;

proc_scheduler_t proc_scheduler;

rbtree_compare_result_t _proc_scheduler_compare(rbtree_node_t *n1, rbtree_node_t *n2) {
    proc_scheduler_context_t *t1 = rbtree_entry(n1, proc_scheduler_context_t, rb_node);
    proc_scheduler_context_t *t2 = rbtree_entry(n2, proc_scheduler_context_t, rb_node);

    return (t1->vruntime >= t2->vruntime) ? RBTREE_COMPARE_GT : RBTREE_COMPARE_LT;
}

proc_thread_t* _proc_scheduler_choose(void) {
    proc_thread_t *thread = NULL;

    // Find the thread with lowest vruntime
    proc_scheduler_context_t *sched = rbtree_entry(rbtree_min(&proc_scheduler.rb_threads),
        proc_scheduler_context_t, rb_node);
    thread = rbtree_entry(sched, proc_thread_t, sched);

    // There should always be at least one runnable thread
    kassert(thread != NULL && thread->state == PROC_THREAD_STATE_RUNNABLE);

    // Remove it from the tree
    thread->state = PROC_THREAD_STATE_RUNNING;
    kassert(rbtree_remove(&proc_scheduler.rb_threads, &thread->sched.rb_node));

    // Update min_vruntime and start time of new thread execution
    proc_scheduler.min_vruntime = thread->sched.vruntime;
    proc_scheduler.exec_start = arch_timer_get_usecs();

    return thread;
}

void proc_scheduler_init(void) {
    spinlock_init(&proc_scheduler.lock);
    rbtree_init(&proc_scheduler.rb_threads);
    proc_scheduler.num_threads = 1;
    proc_scheduler.min_vruntime = 0;
    proc_scheduler.exec_start = 0;
    proc_scheduler.quantum = PROC_SCHEDULER_QUANTUM_US;
}

void proc_scheduler_add(struct proc_thread_s *thread) {
    kassert(thread != NULL);

    // Assumes the given thread has been locked previously
    spinlock_acquire_irq(&proc_scheduler.lock);

    proc_scheduler.num_threads++;

    // Clamp the thread's vruntime to the current minimum in case the thread's been asleep for a long while
    // This prevents the a thread taking up significant CPU time after waking up
    thread->state = PROC_THREAD_STATE_RUNNABLE;
    thread->sched.vruntime = proc_scheduler.min_vruntime;
    kassert(rbtree_insert(&proc_scheduler.rb_threads, _proc_scheduler_compare, &thread->sched.rb_node));

    spinlock_release_irq(&proc_scheduler.lock);
}

void proc_scheduler_remove(struct proc_thread_s *thread) {
    kassert(thread != NULL);

    // Assumes the given thread has been locked previously
    spinlock_acquire_irq(&proc_scheduler.lock);

    proc_scheduler.num_threads--;

    thread->state = PROC_THREAD_STATE_SUSPENDED;
    kassert(rbtree_remove(&proc_scheduler.rb_threads, &thread->sched.rb_node));

    spinlock_release_irq(&proc_scheduler.lock);
}

void proc_scheduler_choose(void) {
    proc_thread_t *thread = NULL;
    proc_thread_t *current = proc_thread_current();

    spinlock_acquire_irq(&proc_scheduler.lock);
    spinlock_acquire_irq(&current->lock);

    // Update the vruntime for the currently running thread and put it back in the tree
    current->state = PROC_THREAD_STATE_RUNNABLE;
    current->sched.vruntime += arch_timer_get_usecs() - proc_scheduler.exec_start;
    kassert(rbtree_insert(&proc_scheduler.rb_threads, _proc_scheduler_compare, &current->sched.rb_node));

    // Get the next thread to run
    thread = _proc_scheduler_choose();

    spinlock_release_irq(&current->lock);
    spinlock_release_irq(&proc_scheduler.lock);

    // Check if we actually need to do a context switch
    if (thread != current) proc_thread_switch(thread);
}

void proc_scheduler_sleep(void) {
    proc_thread_t *thread = NULL;
    proc_thread_t *current = proc_thread_current();

    spinlock_acquire_irq(&proc_scheduler.lock);
    spinlock_acquire_irq(&current->lock);

    // We need to remove the thread from the tree since it is being put to sleep
    proc_scheduler.num_threads--;

    // Update the thread's state and vruntime
    current->state = PROC_THREAD_STATE_SLEEPING;
    current->sched.vruntime += arch_timer_get_usecs() - proc_scheduler.exec_start;

    // Get the next thread to run
    thread = _proc_scheduler_choose();

    spinlock_release_irq(&current->lock);
    spinlock_release_irq(&proc_scheduler.lock);

    proc_thread_switch(thread);
}
