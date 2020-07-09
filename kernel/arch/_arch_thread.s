# This file handles the exception vector table
.text

# x0 [in] - Context to load from
.global arch_thread_context_load
.align 2
arch_thread_context_load:
    ret lr

# x0 [out] - Pointer to context struct where CPU state was saved
.global arch_thread_context_save
.align 2
arch_thread_context_save:
    mov x0, sp
    ret lr
