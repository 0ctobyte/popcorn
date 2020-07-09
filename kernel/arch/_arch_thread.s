# This file handles the exception vector table
.text

# x0 [in] - Context to load from
.global arch_thread_load_context
.align 2
arch_thread_load_context:
    ret lr

# x0 [out] - Pointer to context struct where CPU state was saved
.global arch_thread_save_context
.align 2
arch_thread_save_context:
    mov x0, sp
    ret lr
