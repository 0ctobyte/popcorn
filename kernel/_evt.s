# This file handles the exception vector table
.text
.code 32

# The functions to call on an exception
.comm evt_table, 6*4, 4

# The vector base register requires that the base address is 32 bit aligned
.global exception_vector_table
.align 5
exception_vector_table:
    ldr pc, =reset_exception
    ldr pc, =undefined_exception
    ldr pc, =swi_exception
    ldr pc, =prefetch_abort_exception
    ldr pc, =data_abort_exception
    # Reserved
    b .
    ldr pc, =irq_exception
    ldr pc, =fiq_exception

.align 2
reset_exception:
    b _start

.align 2
undefined_exception:
    stmfd sp!, {r0-r12, lr}
    ldr r0, =evt_table
    ldr r0, [r0]
    teq r0, #0
    beq a0
    blx r0
a0:
    ldmfd sp!, {r0-r12, lr}
    bx lr

.align 2
swi_exception:
    stmfd sp!, {r0-r12, lr}
    ldr r0, =evt_table
    ldr r0, [r0, #4]
    teq r0, #0
    beq a1
    blx r0
a1:
    ldmfd sp!, {r0-r12, lr}
    bx lr

.align 2
prefetch_abort_exception:
    stmfd sp!, {r0-r12, lr}
    ldr r0, =evt_table
    ldr r0, [r0, #8]
    teq r0, #0
    beq a2
    blx r0
a2:
    ldmfd sp!, {r0-r12, lr}
        subs pc, lr, #4

.align 2
data_abort_exception:
    stmfd sp!, {r0-r12, lr, pc}

    # Get the SPSR
    mrs r0, spsr
    str r0, [sp, #-4]!

    # Get useful abort info from the exception status registers
    # DFSR (Data Fault Status Register)
    mrc p15, 0, r0, c5, c0, 0
    str r0, [sp, #-4]!
    # DFAR (Data Fault Address Register)
    mrc p15, 0, r0, c6, c0, 0
    str r0, [sp, #-4]!

    # The register dump on the stack
    mov r0, sp

    ldr r1, =evt_table
    ldr r1, [r1, #12]
    teq r1, #0
    beq a3
    blx r1
a3:
    # Drop the SPSR and exception status register values off the stack
    add sp, sp, #12

    ldmfd sp!, {r0-r12, lr}

    # Drop the PC value off the stack
    add sp, sp, #4

    subs pc, lr, #8

.align 2
irq_exception:
    stmfd sp!, {r0-r12, lr}
    ldr r0, =evt_table
    ldr r0, [r0, #16]
    teq r0, #0
    beq a4
    blx r0
a4:
    ldmfd sp!, {r0-r12, lr}
    subs pc, lr, #4

.align 2
fiq_exception:
    stmfd sp!, {r0-r7, lr}
    ldr r0, =evt_table
    ldr r0, [r0, #20]
    teq r0, #0
    beq a5
    blx r0
a5:
    ldmfd sp!, {r0-r7, lr}
    subs pc, lr, #4

.global evt_init
.type evt_init, %function
.align 2
evt_init:
    # Setup the vector base address register
    ldr r0, =exception_vector_table
    mcr p15, 0, r0, c12, c0, 0

    bx lr

# R0 [in] - Exception type
# R1 [in] - Exception handler address
.global evt_register_handler
.type evt_register_handler, %function
.align 2
evt_register_handler:
    ldr r2, =evt_table
    lsl r0, r0, #2
    str r1, [r2, r0]

    bx lr
