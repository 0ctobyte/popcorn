.text

# x0 - Destination address
# x1 - Kernel physical base address
# x2 - Kernel size
.global _relocate
.align 2
_relocate:
    # Round kernel size up 16 bytes if needed
    ands xzr, x2, #0xf
    beq _relocate_no_size_adjustment
    and x2, x2, #-16
    add x2, x2, #16

_relocate_no_size_adjustment:
    # assuming there's free space after the moved image, let's copy this piece of code there first to avoid trashing this piece of code during the copy
    adr x3, _relocate_copy_loop
    adr x4, _relocate_end
    add x5, x1, x2
    mov x7, x5

_relocate_move_code_loop:
    ldr w6, [x3], #4
    str w6, [x5], #4
    cmp x3, x4
    bne _relocate_move_code_loop

    # Invalidate the icache and then jump!
    mov x5, x0
    mov x6, x1
    ic iallu
    dsb sy
    isb sy
    br x7

_relocate_copy_loop:
    ldp x3, x4, [x1], #16
    stp x3, x4, [x0], #16
    subs x2, x2, #16
    bne _relocate_copy_loop

    ic iallu
    dsb sy
    isb sy

    # Adjust SP
    sub sp, sp, x6
    add sp, sp, x5

    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Update stack frame pointers
    ldp x2, x3, [fp]
_relocate_lr_fp_loop:
    sub x2, x2, x6
    add x2, x2, x5
    sub x3, x3, x6
    add x3, x3, x5
    stp x2, x3, [fp]
    mov fp, x2
    ldp x2, x3, [fp]
    cbnz x2, _relocate_lr_fp_loop

_relocate_done:
    ldp fp, lr, [sp], #16
    ret lr
_relocate_end:
