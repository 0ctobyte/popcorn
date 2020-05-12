.text
# Using the ARM instruction set rather than Thumb
.code 32
.global _start

# Setup the stacks in the BSS. I like my stacks page-aligned pls
# TODO: Should probably make stacks bigger/flexible size
.comm __svc_stack_limit, 4096, 4096
.comm __abt_stack_limit, 4096, 4096
.comm __irq_stack_limit, 4096, 4096
.comm __fiq_stack_limit, 4096, 4096
.comm __und_stack_limit, 4096, 4096
.comm __mon_stack_limit, 4096, 4096

# Setup the system dependent variables in the BSS
.comm PAGESIZE, 4, 4
.comm MEMSIZE, 4, 4
.comm MEMBASEADDR, 4, 4
.comm NUMPAGETABLES, 4, 4
.comm KVIRTUALBASEADDR, 4, 4
.comm PGDPHYSICALBASEADDR, 4, 4
.comm PGTPHYSICALSTARTADDR, 4, 4

# Align to 4 byte (word) boundary, the 2 specifies the # of zeros in the low
# order bits
.align 2
_start:
    # Set the svc stack
    ldr sp, =__svc_stack_limit+4096

    # Get the program status register
    mrs r0, cpsr

    # Set IRQ mode stack, SP (and LR) is banked in IRQ mode
    bic r1, r0, #0x1f
    orr r1, r1, #0x12
    msr cpsr, r1
    ldr sp, =__irq_stack_limit+4096

    # Set up the FIQ mode stack
    bic r1, r0, #0x1f
    orr r1, r1, #0x11
    msr cpsr, r1
    ldr sp, =__fiq_stack_limit+4096

    # Set up the Abort mode stack
    bic r1, r0, #0x1f
    orr r1, r1, #0x17
    msr cpsr, r1
    ldr sp, =__abt_stack_limit+4096

    # Set up the Undefined mode stack
    bic r1, r0, #0x1f
    orr r1, r1, #0x1b
    msr cpsr, r1
    ldr sp, =__und_stack_limit+4096

    # Set up the Monitor mode stack
    bic r1, r0, #0x1f
    orr r1, r1, #0x16
    msr cpsr, r1
    ldr sp, =__mon_stack_limit+4096

    # Go back to supervisor mode
    msr cpsr, r0

    # CPACR: Allow full (PL0 & PL1) access to coprocessors 10 & 11 (VFPU)
    mrc p15, 0, r2, c1, c0, 2
    mov r3, #0xf
    orr r2, r2, r3, lsl #20
    mcr p15, 0, r2, c1, c0, 2
    isb

    # Enable the VFPU
    vmrs r2, fpexc
    mov r3, #1
    orr r2, r2, r3, lsl #30
    vmsr fpexc, r2

    # Jump to kernel main
    bl kmain

    # In case kmain returns
    b .
