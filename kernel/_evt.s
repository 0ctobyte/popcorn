# This file handles the exception vector table
.text

# The functions to call on an exception
.comm evt_table, 15*8, 8

# The vector base register requires that the base address is aligned to 2KB
.global exception_vector_table
.align 10
exception_vector_table:
    # Current exception level with SP_EL0
    # Synchronous
    .align 7
_0:
    stp x0, x1, [sp, #-16]!
    adr x0, _0
    b _exception_entry
    # IRQ
    .align 7
_1:
    stp x0, x1, [sp, #-16]!
    adr x0, _1
    b _exception_entry
    # FIQ
    .align 7
_2:
    stp x0, x1, [sp, #-16]!
    adr x0, _2
    b _exception_entry
    # SError
    .align 7
_3:
    stp x0, x1, [sp, #-16]!
    adr x0, _3
    b _exception_entry

    # Current exception level with SP_ELx
    # Synchronous
    .align 7
_4:
    stp x0, x1, [sp, #-16]!
    adr x0, _4
    b _exception_entry
    # IRQ
    .align 7
_5:
    stp x0, x1, [sp, #-16]!
    adr x0, _5
    b _exception_entry
    # FIQ
    .align 7
_6:
    stp x0, x1, [sp, #-16]!
    adr x0, _6
    b _exception_entry
    # SError
    .align 7
_7:
    stp x0, x1, [sp, #-16]!
    adr x0, _7
    b _exception_entry

    # Lower exception level using aarch64
    # Synchronous
    .align 7
_8:
    stp x0, x1, [sp, #-16]!
    adr x0, _8
    b _exception_entry
    # IRQ
    .align 7
_9:
    stp x0, x1, [sp, #-16]!
    adr x0, _9
    b _exception_entry
    # FIQ
    .align 7
_10:
    stp x0, x1, [sp, #-16]!
    adr x0, _10
    b _exception_entry
    # SError
    .align 7
_11:
    stp x0, x1, [sp, #-16]!
    adr x0, _11
    b _exception_entry

    # Lower exception level using aarch32
    # Synchronous
    .align 7
_12:
    stp x0, x1, [sp, #-16]!
    adr x0, _12
    b _exception_entry
    # IRQ
    .align 7
_13:
    stp x0, x1, [sp, #-16]!
    adr x0, _13
    b _exception_entry
    # FIQ
    .align 7
_14:
    stp x0, x1, [sp, #-16]!
    adr x0, _14
    b _exception_entry
    # SError
    .align 7
_15:
    stp x0, x1, [sp, #-16]!
    adr x0, _15
    b _exception_entry

# x0 - Exception vector address
.align 2
_exception_entry:
    mov x1, sp
    # Assuming x0 & x1 have already been saved on the stack
    # Save state of previous execution
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!

    # Get exception PC from ELR
    mrs x2, ELR_EL1
    stp x30, x2, [sp, #-16]!

    # Calculate index into evt_table
    mrs x2, VBAR_EL1
    sub x0, x0, x2

    # Get the function pointer from the exception vector table
    ldr x2, =evt_table
    ldr x2, [x2, x0, lsl #3]

    # Make sure the function pointer is not NULL
    # x0 holds the pointer to the saved registers on the stack
    # x1 holds the vector base exception offset
    cbz x2, _exception_return
    stp x0, x1, [sp, #-16]
    ldp x1, x0, [sp, #-16]
    blr x2

    b _exception_return

.align 2
_exception_return:
    ldp x30, xzr, [sp], #16
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16

    eret

.global evt_init
.align 2
evt_init:
    # Setup the vector base address register
    ldr x0, =exception_vector_table
    msr VBAR_EL1, x0

    # Enable data and asynchronous aborts
    msr DAIFclr, #0xc

    ret lr

# x0 [in] - Exception type
# x1 [in] - Exception handler address
.global evt_register_handler
.align 2
evt_register_handler:
    ldr x2, =evt_table
    str x1, [x2, x0, lsl #3]
    ret lr
