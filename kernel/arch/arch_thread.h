#ifndef _ARCH_THREAD_H_
#define _ARCH_THREAD_H_

#include <sys/types.h>
#include <kernel/arch/arch_exceptions.h>

struct proc_thread_s;

typedef arch_exception_state_t arch_thread_context_t;

// Initializes the kernel and user thread contexts on the kernel stack
void arch_thread_init(struct proc_thread_s *thread);

// Switch CPU context from old_thread to new_thread, saving current context state on old_thread's kernel stack
void arch_thread_switch(struct proc_thread_s *new_thread, struct proc_thread_s *old_thread);

// Loads the given context into the CPUs register state
void arch_thread_context_load(arch_thread_context_t *context);

// Saves the current CPU context on the current kernel stack returning a pointer to the beginning of the saved context struct
arch_thread_context_t* arch_thread_context_save(void);

// Updates the entry point on the given context
void arch_thread_context_set_entry(arch_thread_context_t *context, uintptr_t entry_point_addr);

// Sets the stack pointer on the given context
void arch_thread_context_set_stack(arch_thread_context_t *context, uintptr_t stack_addr);

#endif // _ARCH_THREAD_H_
