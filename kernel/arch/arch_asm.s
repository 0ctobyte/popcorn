# This file implements some basic functions that can only be implemented through assembly
.text

# Counts the number of bits set
.global arch_popcnt
.align 2
arch_popcnt:
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    mov x1, x0
    mov x0, #0

    # Check if x1 is zero before starting the loop, if so we're done here
    cbz x1, _popcnt_done

    # Get rid of leading zeros so the loop starts counting 1's right away
    clz x2, x1
    lsl x1, x1, x2

_popcnt_loop:
    # Count leading zeros. Since x1 was inverted we are essentially counting the leading ones
    # This count is added to our running sum of bits set
    mvn x1, x1
    clz x2, x1
    add x0, x0, x2

    # Now we need to shift out these bits we just counted as well as the the following contiguous zero bits
    mvn x1, x1
    lsl x1, x1, x2
    clz x2, x1
    lsl x1, x1, x2

    # We're done when x1 is zero
    cbnz x1, _popcnt_loop

_popcnt_done:
    ldp fp, lr, [sp], #16
    ret lr

# Copies from src to dst taking care of overlap
# x0 - dst address
# x1 - src address
# x2 - byte count
.global arch_fast_move
.align 2
arch_fast_move:
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Check if dst < (src + num) && dst > src If so we need to copy backwards
    mov x4, #1
    cmp x1, x0
    b.ge _fast_move_16
    add x3, x1, x2
    cmp x0, x3
    b.ge _fast_move_16
    mov x4, #-1
    add x1, x1, x2
    add x0, x0, x2

_fast_move_16:
    # Check if we can move using 16 byte chunks
    lsr x3, x2, #4
    cbz x3, _fast_move_8

    # Adjust the byte count by the amount of bytes we are going to copy
    mov x5, #16
    msub x2, x3, x5, x2

    cmp x4, #1
    b.eq _fast_move_16_loop

_fast_move_16_loop_backwards:
    ldp x5, x6, [x1, #-16]!
    stp x5, x6, [x0, #-16]!
    subs x3, x3, #1
    b.ne _fast_move_16_loop_backwards
    b _fast_move_8

_fast_move_16_loop:
    ldp x5, x6, [x1], #16
    stp x5, x6, [x0], #16
    subs x3, x3, #1
    b.ne _fast_move_16_loop

_fast_move_8:
    # Check if we can move using 8 byte chunks
    lsr x3, x2, #3
    cbz x3, _fast_move_4

    mov x5, #8
    msub x2, x3, x5, x2

    cmp x4, #1
    b.eq _fast_move_8_loop

_fast_move_8_loop_backwards:
    ldr x5, [x1, #-8]!
    str x5, [x0, #-8]!
    subs x3, x3, #1
    b.ne _fast_move_8_loop_backwards
    b _fast_move_4

_fast_move_8_loop:
    ldr x5, [x1], #8
    str x5, [x0], #8
    subs x3, x3, #1
    b.ne _fast_move_8_loop

_fast_move_4:
    # Check if we can move using 4 byte chunks
    lsr x3, x2, #2
    cbz x3, _fast_move_1

    mov x5, #4
    msub x2, x3, x5, x2

    cmp x4, #1
    b.eq _fast_move_4_loop

_fast_move_4_loop_backwards:
    ldr w5, [x1, #-4]!
    str w5, [x0, #-4]!
    subs x3, x3, #1
    b.ne _fast_move_4_loop_backwards
    b _fast_move_1

_fast_move_4_loop:
    ldr w5, [x1], #4
    str w5, [x0], #4
    subs x3, x3, #1
    b.ne _fast_move_4_loop

_fast_move_1:
    # Check if there's any bytes left to move
    cbz x2, _fast_move_done

    cmp x4, #1
    b.eq _fast_move_4_loop

_fast_move_1_loop_backwards:
    ldrb w5, [x1, #-1]!
    strb w5, [x0, #-1]!
    subs x2, x2, #1
    b.ne _fast_move_1_loop_backwards
    b _fast_move_done

_fast_move_1_loop:
    ldrb w5, [x1], #-1
    strb w5, [x0], #-1
    subs x2, x2, #1
    b.ne _fast_move_1_loop

_fast_move_done:
    ldp fp, lr, [sp], #16
    ret lr

# Sets the dst buffer with all zeros
# x0 - dst
# x1 - byte count
.global arch_fast_zero
.align 2
arch_fast_zero:
    stp fp, lr, [sp, #-16]!
    mov fp, sp

_fast_zero_16:
    # Check if we can zero using 16 byte chunks
    lsr x2, x1, #4
    cbz x2, _fast_zero_8

    # Adjust the byte count by the amount of bytes we are going to copy
    mov x3, #16
    msub x1, x2, x3, x1

_fast_zero_16_loop:
    stp xzr, xzr, [x0], #16
    subs x2, x2, #1
    b.ne _fast_zero_16_loop

_fast_zero_8:
    # Check if we can zero using 8 byte chunks
    lsr x2, x1, #3
    cbz x2, _fast_zero_4

    mov x3, #8
    msub x1, x2, x3, x1

_fast_zero_8_loop:
    str xzr, [x0], #8
    subs x2, x2, #1
    b.ne _fast_zero_8_loop

_fast_zero_4:
    # Check if we can zero using 4 byte chunks
    lsr x2, x1, #2
    cbz x2, _fast_zero_1

    mov x3, #4
    msub x1, x2, x3, x1

_fast_zero_4_loop:
    str wzr, [x0], #4
    subs x2, x2, #1
    b.ne _fast_zero_4_loop

_fast_zero_1:
    # Check if there's any bytes left to zero
    cbz x1, _fast_zero_done

_fast_zero_1_loop:
    strb wzr, [x0], #1
    subs x1, x1, #1
    b.ne _fast_zero_1_loop

_fast_zero_done:
    ldp fp, lr, [sp], #16
    ret lr
