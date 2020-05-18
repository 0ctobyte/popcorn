.text

virtualbaseaddr .req x11
physicalbaseaddr .req x12

.global _start
.align 2
_start:
    # Mask exceptions and interrupts for now
    msr DAIFSet, #0xf

    # Disable the MMU, enable the icache, dcache
    mrs x0, SCTLR_EL1
    mov x1, #1
    bic x0, x0, x1
    mov x1, #0x1004
    orr x0, x0, x1
    msr SCTLR_EL1, x0

    ldr virtualbaseaddr, =__kernel_virtual_start
    adr physicalbaseaddr, _start

    # We don't know what page granule we will eventually end up using so let's relocate ourself to a 64KB page boundary if necessary
    mov x0, physicalbaseaddr
    ldr x1, =__kernel_physical_end
    bl _relocate
    adr physicalbaseaddr, _start

    # Set the el1 stack (for now it's a physical address until we enable the MMU)
    ldr x0, =__el1_stack_limit+8192
    sub x0, x0, virtualbaseaddr
    add sp, x0, physicalbaseaddr

    # Store the physical base address of where the kernel was loaded in memory
    ldr x0, =kernel_physical_start
    sub x0, x0, virtualbaseaddr
    add x0, x0, physicalbaseaddr
    str physicalbaseaddr, [x0]

    # Enable floating point & SIMD
    mrs x0, CPACR_EL1
    mov x1, #0x3
    bfi x0, x1, #20, #2
    msr CPACR_EL1, x0

    # Jump to kernel main
    bl kmain

    # In case kmain returns
    b .

# x0 - Kernel physical base address
# x1 - Kernel size
.align 2
_relocate:
    # We don't need to do anything if we're already 64KB aligned
    ands xzr, x0, #0xffff
    beq _relocate_done

    # Round kernel size up 8 bytes if needed
    ands xzr, x1, #0x7
    beq _relocate_no_size_adjustment
    and x1, x1, #-0x8
    add x1, x1, #0x8

_relocate_no_size_adjustment:
    # Round up to next 64KB page
    and x2, x0, #-0x10000
    add x2, x2, #0x10000

    # We're going to copy backwards because of the overlap
    add x2, x2, x1
    add x0, x0, x1

    # assuming there's free space after the moved image, let's copy this piece of code there first to avoid trashing this piece of code during the copy
    adr x3, _relocate_copy_loop
    adr x4, _relocate_end
    mov x5, x0

_relocate_move_code_loop:
    ldr w6, [x3], #4
    str w6, [x5], #4
    cmp x3, x4
    bne _relocate_move_code_loop

    # Invalidate the icache and then jump!
    ic iallu
    dsb sy
    isb sy
    br x0

_relocate_copy_loop:
    ldp x3, x4, [x0, #-16]!
    stp x3, x4, [x2, #-16]!
    subs x1, x1, #16
    bne _relocate_copy_loop

    ic iallu
    dsb sy
    isb sy

    # Adjust LR
    sub lr, lr, x0
    add lr, lr, x2

_relocate_done:
    ret lr
_relocate_end:

# Setup the boot time stack in the BSS
.comm __el1_stack_limit, 8192, 8

# Setup some boot time variables in the BSS
.comm kernel_physical_start, 8, 8
