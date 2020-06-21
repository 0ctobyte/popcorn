.text

# Atomic test and set, test fails if value is NOT zero
# x0 [in] - Memory location of value to atomically test and set
# x1 [in] - Value to set
# x0 [out] - 0 on success, otherwise failure
.global atomic_test_and_set
.align 2
atomic_test_and_set:
    mov x2, x1
    mov x1, x0
    ldxr x0, [x1]
    cbnz x0, _atomic_test_and_set_exit
    stxr w0, x2, [x1]

_atomic_test_and_set_exit:
    ret lr

# Atomically test and set specified bit, test fails if bit is not zero
# x0 [in] - Memory location of value to atomically test and set
# x1 [in] - Bit to set
# x0 [out] - 0 on success, otherwise failure
.global atomic_test_and_set_bit
.align 2
atomic_test_and_set_bit:
    mov x2, x1
    mov x1, x0
    ldxr x0, [x1]
    tst x0, x2
    bne _atomic_test_and_set_bit_exit
    orr x2, x0, x2
    stxr w0, x2, [x1]

_atomic_test_and_set_bit_exit:
    ret lr

# Atomically increment the value
# x0 [in] - Memory location of atomic value
# x0 [out] - Value before increment
.global atomic_inc
.align 2
atomic_inc:
    ldxr x1, [x0]
    add x2, x1, #1
    stxr w2, x2, [x0]
    cbnz w2, atomic_inc

    mov x0, x1
    ret lr

# Atomically decrement the value
# x0 [in] - Memory location of atomic value
# x0 [out] - Value before decrement
.global atomic_dec
.align 2
atomic_dec:
    ldxr x1, [x0]
    sub x2, x1, #1
    stxr w2, x2, [x0]
    cbnz w2, atomic_dec

    mov x0, x1
    ret lr
