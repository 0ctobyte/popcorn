.text

# Called by threads running for the first time
# x1 [in] - New thread pointer
# x2 [in] - Old thread pointer
.global _arch_thread_run_stub
.align 2
_arch_thread_run_stub:
    mov x0, x1
    mov x1, x2
    stp x29, lr, [sp, #-16]!
    bl _arch_thread_run
    ldp x29, lr, [sp], #16

    # We don't return from _arch_exception_return
    b _arch_exception_return

# x0 [in]  - Context to load from
# x0 [out] - Should be -1 on return
.global arch_thread_load_context
.align 2
arch_thread_load_context:
    ldp x0, x1, [x0, #0x0]
    ldp x2, x3, [x0, #0x10]
    ldp x4, x5, [x0, #0x20]
    ldp x6, x7, [x0, #0x30]
    ldp x8, x9, [x0, #0x40]
    ldp x10, x11, [x0, #0x50]
    ldp x12, x13, [x0, #0x60]
    ldp x14, x15, [x0, #0x70]
    ldp x16, x17, [x0, #0x80]
    ldp x18, x19, [x0, #0x90]
    ldp x20, x21, [x0, #0xa0]
    ldp x22, x23, [x0, #0xb0]
    ldp x24, x25, [x0, #0xc0]
    ldp x26, x27, [x0, #0xd0]
    ldp x28, x29, [x0, #0xe0]
    ldp x30, xzr, [x0, #0xf0]
    ldr x0, [x0, #0xf8]
    mov sp, x0

    mvn x0, xzr
    ret lr

# x0 [in]  - Context to save to
# x0 [out] - Should be 0 on return
.global arch_thread_save_context
.align 2
arch_thread_save_context:
    stp x0, x1, [x0, #0x0]
    stp x2, x3, [x0, #0x10]
    stp x4, x5, [x0, #0x20]
    stp x6, x7, [x0, #0x30]
    stp x8, x9, [x0, #0x40]
    stp x10, x11, [x0, #0x50]
    stp x12, x13, [x0, #0x60]
    stp x14, x15, [x0, #0x70]
    stp x16, x17, [x0, #0x80]
    stp x18, x19, [x0, #0x90]
    stp x20, x21, [x0, #0xa0]
    stp x22, x23, [x0, #0xb0]
    stp x24, x25, [x0, #0xc0]
    stp x26, x27, [x0, #0xd0]
    stp x28, x29, [x0, #0xe0]
    stp x30, xzr, [x0, #0xf0]
    mov x1, sp
    str x1, [x0, #0xf8]

    mov x0, xzr
    ret lr
