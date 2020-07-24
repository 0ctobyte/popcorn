/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/kmem_slab.h>
#include <kernel/hash.h>
#include <kernel/vm/vm_types.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/arch_atomic.h>
#include <kernel/arch/arch_thread.h>
#include <kernel/proc/proc_scheduler.h>
#include <kernel/proc/proc_task.h>
#include <kernel/proc/proc_thread.h>

atomic_t tid;
#define TID_ALLOC() (arch_atomic_inc(&tid))

size_t kernel_stack_size;
proc_thread_t *current_thread;
proc_thread_t thread_template;

// proc_thread_t slab
#define PROC_THREAD_SLAB_NUM (1024)
kmem_slab_t proc_thread_slab;

// Kernel stack slab
#define KERNEL_STACK_SLAB_NUM (1024)
kmem_slab_t kernel_stack_slab;

#define NUM_BUCKETS              (1024)
#define PROC_EVENT_HASH(event)   (hash64_fnv1a((uint64_t)event) % NUM_BUCKETS)

typedef struct {
    spinlock_t lock[NUM_BUCKETS];
    list_t ll_threads[NUM_BUCKETS];
} proc_thread_event_hash_table_t;

proc_thread_event_hash_table_t event_table;

void proc_thread_init(void) {
    // Initialize the slab for allocating proc_thread_t structs
    kmem_slab_create(&proc_thread_slab, sizeof(proc_thread_t), PROC_THREAD_SLAB_NUM);

    // Initialize the slab for allocating kernel stacks. Kernel stacks are the size of one page
    kernel_stack_size = PAGESIZE;
    kmem_slab_create(&kernel_stack_slab, kernel_stack_size, KERNEL_STACK_SLAB_NUM);

    // Initialize the event hash table
    arch_fast_zero(event_table.lock, sizeof(spinlock_t) * NUM_BUCKETS);
    arch_fast_zero(event_table.ll_threads, sizeof(list_t) * NUM_BUCKETS);

    // Setup the thread template object
    spinlock_init(&thread_template.lock);
    thread_template.tid = 0;
    thread_template.task = proc_task_kernel();
    thread_template.state = PROC_THREAD_STATE_SUSPENDED;
    thread_template.suspend_cnt = 1;
    thread_template.refcnt = 1;
    thread_template.event = 0;
    list_node_init(&thread_template.ll_enode);
    list_node_init(&thread_template.ll_tnode);
    thread_template.kernel_stack = NULL;
    rbtree_node_init(&thread_template.sched.rb_node);
    thread_template.sched.vruntime = 0;

    // Create a thread for the currently running kernel code
    proc_thread_t *kernel_thread = kmem_slab_alloc(&proc_thread_slab);
    kassert(kernel_thread != NULL);

    int stack_var;
    *kernel_thread = thread_template;
    kernel_thread->tid = TID_ALLOC();
    kernel_thread->state = PROC_THREAD_STATE_RUNNING;
    kernel_thread->suspend_cnt = 0;
    kernel_thread->kernel_stack = (void*)ROUND_PAGE_DOWN(&stack_var);
    rbtree_node_init(&kernel_thread->sched.rb_node);
    kernel_thread->sched.vruntime = 0;

    spinlock_acquire_irq(&proc_task_kernel()->lock);
    kassert(list_insert_last(&proc_task_kernel()->ll_threads, &kernel_thread->ll_tnode));
    proc_task_kernel()->num_threads++;
    spinlock_release_irq(&proc_task_kernel()->lock);

    proc_thread_current() = kernel_thread;
}

kresult_t proc_thread_create(proc_task_t *task, proc_thread_t **thread) {
    if (task == NULL || thread == NULL) return KRESULT_INVALID_ARGUMENT;

    void *kernel_stack = kmem_slab_alloc(&kernel_stack_slab);
    if (kernel_stack == NULL) return KRESULT_RESOURCE_SHORTAGE;

    proc_thread_t *new_thread = kmem_slab_alloc(&proc_thread_slab);
    if (new_thread == NULL) {
        kmem_slab_free(&kernel_stack_slab, kernel_stack);
        return KRESULT_RESOURCE_SHORTAGE;
    }

    *new_thread = thread_template;
    new_thread->tid = TID_ALLOC();
    new_thread->task = task;
    new_thread->kernel_stack = kernel_stack;

    // Initialize the thread context
    arch_thread_init(new_thread);

    spinlock_acquire_irq(&new_thread->task->lock);

    // Thread starts out in suspended state
    new_thread->suspend_cnt = new_thread->task->suspend_cnt + 1;

    // If this is a kernel thread, set the privilege level to kernel mode to prevent switching
    // into user mode on exception_return
    if (new_thread->task == proc_task_kernel()) {
        arch_thread_set_privilege(new_thread, ARCH_THREAD_PRIVILEGE_KERNEL);
    }

    // Add the thread to task
    kassert(list_insert_last(&new_thread->task->ll_threads, &new_thread->ll_tnode));
    new_thread->task->num_threads++;

    spinlock_release_irq(&new_thread->task->lock);

    *thread = new_thread;
    return KRESULT_OK;
}

void proc_thread_reference(proc_thread_t *thread) {
    kassert(thread != NULL);

    spinlock_acquire_irq(&thread->lock);
    thread->refcnt++;
    spinlock_release_irq(&thread->lock);
}

void proc_thread_unreference(proc_thread_t *thread) {
    kassert(thread != NULL);

    spinlock_acquire_irq(&thread->lock);

    thread->refcnt--;

    // FIXME need to terminate or suspend a thread
    if (thread->refcnt == 0) {
        spinlock_acquire_irq(&thread->task->lock);
        kassert(list_remove(&thread->task->ll_threads, &thread->ll_tnode));
        spinlock_release_irq(&thread->task->lock);

        kmem_slab_free(&kernel_stack_slab, thread->kernel_stack);
        kmem_slab_free(&proc_thread_slab, thread);
    }

    spinlock_release_irq(&thread->lock);
}

kresult_t proc_thread_terminate(proc_thread_t *thread) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    return KRESULT_UNIMPLEMENTED;
}

kresult_t proc_thread_resume(proc_thread_t *thread) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_acquire_irq(&thread->lock);

    thread->suspend_cnt--;
    if (thread->suspend_cnt == 0) proc_scheduler_add(thread);

    spinlock_release_irq(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_suspend(proc_thread_t *thread) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    // FIXME The next time this thread enters the kernel, it'll be stopped and marked SUSPENDED
    spinlock_acquire_irq(&thread->lock);
    thread->suspend_cnt++;
    spinlock_release_irq(&thread->lock);

    return KRESULT_OK;
}

void proc_thread_switch(proc_thread_t *new_thread) {
    kassert(new_thread != NULL);

    if (proc_thread_current() == new_thread) {
        spinlock_acquire_irq(&new_thread->lock);
    } else if (proc_thread_current() < new_thread) {
        spinlock_acquire_irq(&proc_thread_current()->lock);
        spinlock_acquire_irq(&new_thread->lock);
    } else {
        spinlock_acquire_irq(&new_thread->lock);
        spinlock_acquire_irq(&proc_thread_current()->lock);
    }

    new_thread = arch_thread_switch(new_thread, proc_thread_current());
    proc_thread_t *cur_thread = proc_thread_current();
    proc_thread_current() = new_thread;

    if (cur_thread == new_thread) {
        spinlock_release_irq(&new_thread->lock);
    } else if (cur_thread < new_thread) {
        spinlock_release_irq(&cur_thread->lock);
        spinlock_release_irq(&new_thread->lock);
    } else {
        spinlock_release_irq(&new_thread->lock);
        spinlock_release_irq(&cur_thread->lock);
    }
}

void proc_thread_sleep(proc_event_t event, spinlock_t *interlock, bool interruptible) {
    kassert(interlock != NULL);

    uint64_t hash_bkt = PROC_EVENT_HASH(event);
    proc_thread_t *current = proc_thread_current();

    spinlock_acquire_irq(&event_table.lock[hash_bkt]);
    spinlock_acquire_irq(&current->lock);

    // Add this thread to event hash table
    current->event = event;
    kassert(list_insert_last(&event_table.ll_threads[hash_bkt], &current->ll_enode));

    // Now we can unlock the interlock
    spinlock_release_irq(interlock);

    spinlock_release_irq(&current->lock);
    spinlock_release_irq(&event_table.lock[hash_bkt]);

    // Call the scheduler
    proc_scheduler_sleep();
}

void proc_thread_wake(proc_event_t event) {
    uint64_t hash_bkt = PROC_EVENT_HASH(event);

    spinlock_acquire_irq(&event_table.lock[hash_bkt]);

    // Wake up all threads sleeping on this event
    proc_thread_t *thread = NULL;
    list_for_each_entry(&event_table.ll_threads[hash_bkt], thread, ll_enode) {
        if (thread->event == event) proc_scheduler_wake(thread);
    }

    spinlock_release_irq(&event_table.lock[hash_bkt]);
}

void proc_thread_wake_one(proc_event_t event) {
    uint64_t hash_bkt = PROC_EVENT_HASH(event);

    spinlock_acquire_irq(&event_table.lock[hash_bkt]);

    // Wake up the first thread sleeping on this event
    proc_thread_t *thread = NULL;
    list_for_each_entry(&event_table.ll_threads[hash_bkt], thread, ll_enode) {
        if (thread->event == event) {
            proc_scheduler_wake(thread);
            break;
        }
    }

    spinlock_release_irq(&event_table.lock[hash_bkt]);
}

kresult_t proc_thread_set_context(proc_thread_t *thread, arch_thread_context_t *context) {
    if (thread == NULL || context == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_acquire_irq(&thread->lock);
    arch_fast_move(&thread->context, context, sizeof(arch_thread_context_t));
    spinlock_release_irq(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_get_context(proc_thread_t *thread, arch_thread_context_t *context) {
    if (thread == NULL || context == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_acquire_irq(&thread->lock);
    arch_fast_move(context, &thread->context, sizeof(arch_thread_context_t));
    spinlock_release_irq(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_set_entry(proc_thread_t *thread, void *entry) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_acquire_irq(&thread->lock);
    arch_thread_set_entry(thread, entry);
    spinlock_release_irq(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_set_stack(proc_thread_t *thread, void *user_stack) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_acquire_irq(&thread->lock);
    arch_thread_set_stack(thread, user_stack);
    spinlock_release_irq(&thread->lock);

    return KRESULT_OK;
}
