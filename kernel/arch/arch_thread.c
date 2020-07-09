#include <kernel/kassert.h>
#include <kernel/proc/proc_thread.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/arch_thread.h>

void arch_thread_init(struct proc_thread_s *thread) {
    kassert(thread != NULL && thread->kernel_stack != 0);

    // Make room on the kernel stack for the user thread context
    arch_context_t *user = (arch_context_t*)(thread->kernel_stack + kernel_stack_size - sizeof(arch_context_t));
    arch_context_t *kernel = (arch_context_t*)((uintptr_t)user - sizeof(arch_context_t));
    thread->context.kernel_stack_top = (void*)kernel;
    arch_fast_zero(kernel, 2 * sizeof(arch_context_t));
}
