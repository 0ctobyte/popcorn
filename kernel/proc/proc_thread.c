#include <kernel/kassert.h>
#include <kernel/kmem_slab.h>
#include <kernel/vm/vm_types.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/arch_atomic.h>
#include <kernel/arch/arch_thread.h>
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

void proc_thread_init(void) {
    // Initialize the slab for allocating proc_thread_t structs
    kmem_slab_create(&proc_thread_slab, sizeof(proc_thread_t), PROC_THREAD_SLAB_NUM);

    // Initialize the slab for allocating kernel stacks. Kernel stacks are the size of one page
    kernel_stack_size = PAGESIZE;
    kmem_slab_create(&kernel_stack_slab, kernel_stack_size, KERNEL_STACK_SLAB_NUM);

    // Setup the thread template object
    thread_template.lock = SPINLOCK_INIT;
    thread_template.tid = 0;
    thread_template.task = proc_task_kernel();
    thread_template.state = PROC_THREAD_STATE_SUSPENDED;
    thread_template.suspend_cnt = 1;
    thread_template.priority = proc_task_kernel()->priority;
    thread_template.sched_priority = proc_task_kernel()->priority;
    thread_template.refcnt = 1;
    thread_template.event = 0;
    thread_template.ll_enode = LIST_NODE_INITIALIZER;
    thread_template.ll_tnode = LIST_NODE_INITIALIZER;
    thread_template.ll_qnode = LIST_NODE_INITIALIZER;
    thread_template.kernel_stack = NULL;

    // Create a thread for the currently running kernel code
    proc_thread_t *kernel_thread = kmem_slab_alloc(&proc_thread_slab);
    kassert(kernel_thread != NULL);

    int stack_var;
    *kernel_thread = thread_template;
    kernel_thread->tid = TID_ALLOC();
    kernel_thread->state = PROC_THREAD_STATE_RUNNING;
    kernel_thread->suspend_cnt = 0;
    kernel_thread->kernel_stack = (void*)ROUND_PAGE_DOWN(&stack_var);

    spinlock_irqacquire(&proc_task_kernel()->lock);
    kassert(list_insert_last(&proc_task_kernel()->ll_threads, &kernel_thread->ll_tnode));
    proc_task_kernel()->num_threads++;
    spinlock_irqrelease(&proc_task_kernel()->lock);

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

    spinlock_irqacquire(&new_thread->task->lock);

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

    spinlock_irqrelease(&new_thread->task->lock);

    *thread = new_thread;
    return KRESULT_OK;
}

void proc_thread_reference(proc_thread_t *thread) {
    kassert(thread != NULL);

    spinlock_irqacquire(&thread->lock);
    thread->refcnt++;
    spinlock_irqrelease(&thread->lock);
}

void proc_thread_unreference(proc_thread_t *thread) {
    kassert(thread != NULL);

    spinlock_irqacquire(&thread->lock);

    thread->refcnt--;

    // FIXME need to terminate or suspend a thread
    if (thread->refcnt == 0) {
        spinlock_irqacquire(&thread->task->lock);
        kassert(list_remove(&thread->task->ll_threads, &thread->ll_tnode));
        spinlock_irqrelease(&thread->task->lock);

        kmem_slab_free(&kernel_stack_slab, thread->kernel_stack);
        kmem_slab_free(&proc_thread_slab, thread);
    }

    spinlock_irqrelease(&thread->lock);
}

kresult_t proc_thread_terminate(proc_thread_t *thread) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    return KRESULT_UNIMPLEMENTED;
}

kresult_t proc_thread_resume(proc_thread_t *thread) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_irqacquire(&thread->lock);

    thread->suspend_cnt--;
    if (thread->suspend_cnt == 0) thread->state = PROC_THREAD_STATE_RUNNABLE;

    spinlock_irqrelease(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_suspend(proc_thread_t *thread) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    // The next time this thread returns from a trap, it'll be stopped and marked SUSPENDED
    spinlock_irqacquire(&thread->lock);
    thread->suspend_cnt++;
    spinlock_irqrelease(&thread->lock);

    return KRESULT_OK;
}

void proc_thread_switch(proc_thread_t *new_thread) {
    kassert(new_thread != NULL);

    if (proc_thread_current() == new_thread) {
        spinlock_irqacquire(&new_thread->lock);
    } else if (proc_thread_current() < new_thread) {
        spinlock_irqacquire(&proc_thread_current()->lock);
        spinlock_irqacquire(&new_thread->lock);
    } else {
        spinlock_irqacquire(&new_thread->lock);
        spinlock_irqacquire(&proc_thread_current()->lock);
    }

    new_thread = arch_thread_switch(new_thread, proc_thread_current());
    proc_thread_t *cur_thread = proc_thread_current();
    proc_thread_current() = new_thread;

    if (cur_thread == new_thread) {
        spinlock_irqrelease(&new_thread->lock);
    } else if (cur_thread < new_thread) {
        spinlock_irqrelease(&cur_thread->lock);
        spinlock_irqrelease(&new_thread->lock);
    } else {
        spinlock_irqrelease(&new_thread->lock);
        spinlock_irqrelease(&cur_thread->lock);
    }
}

void proc_thread_sleep(proc_thread_t *thread, proc_event_t event, bool interruptible) {
    kassert(thread != NULL);

    // FIXME Add this thread to the event hash table and mark it blocked and call the scheduler
    spinlock_irqacquire(&thread->lock);
    spinlock_irqrelease(&thread->lock);
}

void proc_thread_wake(proc_thread_t *thread) {
    kassert(thread != NULL);

    // FIXME Remove this thread from the event hash table and mark it runnable, call the scheduler
    spinlock_irqacquire(&thread->lock);
    spinlock_irqrelease(&thread->lock);
}

kresult_t proc_thread_set_context(proc_thread_t *thread, arch_thread_context_t *context) {
    if (thread == NULL || context == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_irqacquire(&thread->lock);
    arch_fast_move(&thread->context, context, sizeof(arch_thread_context_t));
    spinlock_irqrelease(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_get_context(proc_thread_t *thread, arch_thread_context_t *context) {
    if (thread == NULL || context == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_irqacquire(&thread->lock);
    arch_fast_move(context, &thread->context, sizeof(arch_thread_context_t));
    spinlock_irqrelease(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_set_entry(proc_thread_t *thread, void *entry) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_irqacquire(&thread->lock);
    arch_thread_set_entry(thread, entry);
    spinlock_irqrelease(&thread->lock);

    return KRESULT_OK;
}

kresult_t proc_thread_set_stack(proc_thread_t *thread, void *user_stack) {
    if (thread == NULL) return KRESULT_INVALID_ARGUMENT;

    spinlock_irqacquire(&thread->lock);
    arch_thread_set_stack(thread, user_stack);
    spinlock_irqrelease(&thread->lock);

    return KRESULT_OK;
}
