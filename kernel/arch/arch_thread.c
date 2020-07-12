#include <kernel/proc/proc_task.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/pmap.h>
#include <kernel/arch/arch_thread.h>

extern void _arch_thread_run_stub(struct proc_thread_s *new_thread, struct proc_thread_s *old_thread);

void _arch_thread_run(struct proc_thread_s *new_thread, struct proc_thread_s *old_thread) {
    // For threads running for the first time, we need to set the current thread and release the locks
    proc_thread_current() = new_thread;

    if (old_thread == new_thread) {
        spinlock_irq_release(&new_thread->lock);
    } else if (old_thread < new_thread) {
        spinlock_irq_release(&old_thread->lock);
        spinlock_irq_release(&new_thread->lock);
    } else {
        spinlock_irq_release(&new_thread->lock);
        spinlock_irq_release(&old_thread->lock);
    }
}

void arch_thread_init(struct proc_thread_s *thread) {
    // Assuming thread and kernel_stack have been setup already by proc_thread code
    // Make room on the kernel stack for the user thread context an kernel thread context
    arch_context_t *user = (arch_context_t*)(thread->kernel_stack + kernel_stack_size - sizeof(arch_context_t));
    arch_context_t *kernel = (arch_context_t*)((uintptr_t)user - sizeof(arch_context_t));
    arch_fast_zero(kernel, 2 * sizeof(arch_context_t));

    thread->context.context = user;
    thread->context.kernel_stack_top = (void*)kernel;

    // New threads will go through the _arch_thread_run_stub code before executing _arch_exception_return
    // which will load the user or kernel context and eret to the entry point for the thread
    kernel->lr = (uint64_t)((void*)_arch_thread_run_stub);
}

struct proc_thread_s* arch_thread_switch(struct proc_thread_s *new_thread, struct proc_thread_s *old_thread) {
    // If we are switching to a different task we need to switch pmap's
    if (old_thread->task != new_thread->task) {
        pmap_deactivate(old_thread->task->vm_map->pmap);
        pmap_activate(new_thread->task->vm_map->pmap);
    }

    // Assuming both threads have been locked already
    void *kernel_stack_top = arch_thread_save_context();

    // Load context will bring us here with kernel_stack_top == 0
    // old_thread is actually the new thread we switched to at this point
    if (kernel_stack_top == 0) return old_thread;

    // Otherwise we have saved the context on the current thread's kernel stack
    old_thread->context.kernel_stack_top = (arch_context_t*)kernel_stack_top;

    // Check if this is the first run of this thread, if so set the parameters for _arch_thread_run
    arch_context_t *thread_context = (arch_context_t*)new_thread->context.kernel_stack_top;

    if (thread_context->lr == (uint64_t)((void*)_arch_thread_run_stub)) {
        thread_context->x[1] = (uint64_t)new_thread;
        thread_context->x[2] = (uint64_t)old_thread;
    }

    // Load the context from the new thread which will run the new thread on this CPU at the if statement a couple of
    // lines earlier. Unless this thread is running for the first in which case it will start execution at
    // _arch_thread_run_stub
    arch_thread_load_context(thread_context);
}

void arch_thread_set_entry(struct proc_thread_s *thread, void *entry) {
    thread->context.context->elr = (uint64_t)entry;
}

void arch_thread_set_stack(struct proc_thread_s *thread, void *user_stack) {
    thread->context.context->sp = (uint64_t)user_stack;
}

void arch_thread_set_privilege(struct proc_thread_s *thread, arch_thread_privilege_t privilege) {
    // User mode is spsr[4] = 0 (AARCH64) and spsr[3:0] = 0
    thread->context.context->spsr &= ~0x1f;

    // Kernel mode is spsr[4] = 0 (AARCH64) and spsr[3:0] = 5 (using kernel stack in EL1)
    if (privilege = ARCH_THREAD_PRIVILEGE_KERNEL) thread->context.context->spsr = 0x5;
}
