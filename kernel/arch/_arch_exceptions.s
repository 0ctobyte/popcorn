# This file handles the exception vector table
.text

# The vector base register requires that the base address is aligned to 2KB
.global arch_exception_vector_table
.align 11
arch_exception_vector_table:
    # Current exception level with SP_EL0
    # Synchronous
    .align 7
_0:
    stp x0, x1, [sp, #-16]!
    adr x0, _0
    mov x1, sp
    b _arch_exception_entry
    # IRQ
    .align 7
_1:
    stp x0, x1, [sp, #-16]!
    adr x0, _1
    mov x1, sp
    b _arch_exception_entry
    # FIQ
    .align 7
_2:
    stp x0, x1, [sp, #-16]!
    adr x0, _2
    mov x1, sp
    b _arch_exception_entry
    # SError
    .align 7
_3:
    stp x0, x1, [sp, #-16]!
    adr x0, _3
    mov x1, sp
    b _arch_exception_entry

    # Current exception level with SP_ELx
    # Synchronous
    .align 7
_4:
    stp x0, x1, [sp, #-16]!
    adr x0, _4
    mov x1, sp
    b _arch_exception_entry
    # IRQ
    .align 7
_5:
    stp x0, x1, [sp, #-16]!
    adr x0, _5
    mov x1, sp
    b _arch_exception_entry
    # FIQ
    .align 7
_6:
    stp x0, x1, [sp, #-16]!
    adr x0, _6
    mov x1, sp
    b _arch_exception_entry
    # SError
    .align 7
_7:
    stp x0, x1, [sp, #-16]!
    adr x0, _7
    mov x1, sp
    b _arch_exception_entry

    # Lower exception level using aarch64
    # Synchronous
    .align 7
_8:
    stp x0, x1, [sp, #-16]!
    adr x0, _8
    mrs x1, SP_EL0
    b _arch_exception_entry
    # IRQ
    .align 7
_9:
    stp x0, x1, [sp, #-16]!
    adr x0, _9
    mrs x1, SP_EL0
    b _arch_exception_entry
    # FIQ
    .align 7
_10:
    stp x0, x1, [sp, #-16]!
    adr x0, _10
    mrs x1, SP_EL0
    b _arch_exception_entry
    # SError
    .align 7
_11:
    stp x0, x1, [sp, #-16]!
    adr x0, _11
    mrs x1, SP_EL0
    b _arch_exception_entry

    # Lower exception level using aarch32
    # Synchronous
    .align 7
_12:
    stp x0, x1, [sp, #-16]!
    adr x0, _12
    mrs x1, SP_EL0
    b _arch_exception_entry
    # IRQ
    .align 7
_13:
    stp x0, x1, [sp, #-16]!
    adr x0, _13
    mrs x1, SP_EL0
    b _arch_exception_entry
    # FIQ
    .align 7
_14:
    stp x0, x1, [sp, #-16]!
    adr x0, _14
    mrs x1, SP_EL0
    b _arch_exception_entry
    # SError
    .align 7
_15:
    stp x0, x1, [sp, #-16]!
    adr x0, _15
    mrs x1, SP_EL0
    b _arch_exception_entry

# x0 - Exception vector address
.align 2
_arch_exception_entry:
    # x0 & x1 have already been saved on the stack. Save state of previous execution
    # x0 holds the vector address and x1 holds the appropriate stack pointer before exception was taken
    # Make some room for the PC and SPSR. FAR and ESR will overwrite x0 & x1 locations on the stack after they are moved to the top of stack
    stp xzr, xzr, [sp, #-16]!
    stp x30, x1, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x2, x3, [sp, #-16]!

    # Move x0, x1 from bottom of stack to top of stack
    ldp x2, x3, [x1]
    stp x2, x3, [sp, #-16]!

    # Get exception PC from ELR and SPSR
    mrs x2, ELR_EL1
    mrs x3, SPSR_EL1
    stp x2, x3, [x1, #-16]

    # Get the FAR and ESR
    mrs x2, FAR_EL1
    mrs x3, ESR_EL1
    stp x2, x3, [x1]

    # Calculate index into exception_table
    mrs x2, VBAR_EL1
    sub x0, x0, x2
    lsr x0, x0, #7

    # Make sure the function pointer is not NULL
    # x0 holds the vector base exception index
    # x1 holds the pointer to the saved registers on the stack
    cbz x2, _arch_exception_return
    mov x1, sp
    adr x2, arch_exception_handler
    blr x2

    b _arch_exception_return

.align 2
_arch_exception_return:
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

    # Save x0 and x1 by overwriting FAR and ESR on the stack context. We'll load these again later
    stp x0, x1, [sp, #32]

    # Load LR and SP
    ldp x30, x0, [sp], #16

    # Load ELR
    ldr x1, [sp], #8
    msr ELR_EL1, x1

    # Load SPSR
    ldr x1, [sp], #8
    msr SPSR_EL1, x1

    tbnz x1, #0, _skip_sp_el0_load
    msr SP_EL0, x0
_skip_sp_el0_load:
    mov sp, x0

    # Re-load x0 and x1
    ldp x0, x1, [sp], #16

    eret

.global arch_exceptions_init
.align 2
arch_exceptions_init:
    # Setup the vector base address register
    ldr x0, =arch_exception_vector_table
    msr VBAR_EL1, x0

    # Enable data and asynchronous aborts
    msr DAIFclr, #0xc

    ret lr
