# This file implements some basic functions that can only be implemented through assembly
.text

# Unsigned integer modulo
# operand1 % operand2
# r0 [in] - operand1
# r1 [in] - operand2
# r0 [out] - The modulo value
.global _umod
.align 2
_umod:
    udiv x2, x0, x1
    msub x0, x2, x1, x0
    ret lr

# Count trailing zeros
# Returns the number of trailing zeros
.global _ctz
.align 2
_ctz:
    rbit x0, x0
    clz x0, x0
    ret lr

# Count leading zeros
# Returns the number of leading zeros
.global _clz
.align 2
_clz:
    clz x0, x0
    ret lr

# Reverse bit order
# Returns the 64-bit integer in reversed bit order
.global _rbit
.align 2
_rbit:
    rbit x0, x0
    ret lr

# Reverse byte order in a 32-bit word. Use to convert between big and little endian
.global _rev32
.align 2
_rev32:
    rev w0, w0
    ret lr

# Reverse byte order in a 64-bit word. Use to convert between big and little endian
.global _rev64
.align 2
_rev64:
    rev x0, x0
    ret lr

# Counts the number of bits set
.global _popcnt
.align 2
_popcnt:
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

# Copies pages from src to dst
# x0 - dst address
# x1 - src address
# x2 - multiple of page size
.global _copy_pages
.align 2
_copy_pages:
    stp fp, lr, [sp, #-16]!
    mov fp, sp

_copy_pages_loop:
    ldp x3, x4, [x1], #16
    stp x3, x4, [x0], #16
    sub x2, x2, #16
    cbnz x2, _copy_pages_loop

    ldp fp, lr, [sp], #16
    ret lr

# Zero out pages
# x0 - address
# x1 - multiple of page size
.global _zero_pages
.align 2
_zero_pages:
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    mov x2, xzr
    mov x3, xzr
_zero_pages_loop:
    stp x2, x3, [x0], #16
    sub x1, x1, #16
    cbnz x1, _zero_pages_loop

    ldp fp, lr, [sp], #16
    ret lr
