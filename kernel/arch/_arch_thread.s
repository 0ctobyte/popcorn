.text

# x0 [in]  - Context to load from
# x0 [out] - Should be zero on successful load
.global arch_thread_load_context
.align 2
arch_thread_load_context:
    mov sp, x0

    ldp x0, x1, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x28, x29, [sp], #16
    ldp x30, xzr, [sp], #16
    add sp, sp, #32

    mov x0, xzr
    ret lr

# x0 [out] - Pointer to where context is stored
.global arch_thread_save_context
.align 2
arch_thread_save_context:
    mov x0, sp

    // First four words on stack are FAR, ESR, ELR and SPSR. These are zero'd out
    stp xzr, xzr, [x0, #-16]!
    stp xzr, xzr, [x0, #-16]!
    stp x30, xzr, [x0, #-16]!
    stp x28, x29, [x0, #-16]!
    stp x26, x27, [x0, #-16]!
    stp x24, x25, [x0, #-16]!
    stp x22, x23, [x0, #-16]!
    stp x20, x21, [x0, #-16]!
    stp x18, x19, [x0, #-16]!
    stp x16, x17, [x0, #-16]!
    stp x14, x15, [x0, #-16]!
    stp x12, x13, [x0, #-16]!
    stp x10, x11, [x0, #-16]!
    stp x8, x9, [x0, #-16]!
    stp x6, x7, [x0, #-16]!
    stp x4, x5, [x0, #-16]!
    stp x2, x3, [x0, #-16]!
    stp x0, x1, [x0, #-16]
    sub x0, x0, #16

    ret lr
