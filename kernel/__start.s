.text

virtualbaseaddr .req x6
physicalbaseaddr .req x7

.global _start
.align 2
_start:
    # Mask exceptions and interrupts for now
    msr DAIFSet, #0xf

    # Disable the MMU, enable the icache, dcache
    mov x0, #0x1004
    msr SCTLR_EL1, x0

    ldr virtualbaseaddr, =__kernel_virtual_start
    adr physicalbaseaddr, _start

    # Store the physical base address of where the kernel was loaded in memory
    ldr x0, =kernel_physical_start
    sub x0, x0, virtualbaseaddr
    add x0, x0, physicalbaseaddr
    str physicalbaseaddr, [x0]

    # Set the el1 stack (for now it's a physical address until we enable the MMU)
    ldr x0, =__el1_stack_limit+4096
    sub x0, x0, virtualbaseaddr
    add sp, x0, physicalbaseaddr

    # Enable floating point & SIMD
    mrs x0, CPACR_EL1
    mov x1, #0x3
    bfi x0, x1, #20, #2
    msr CPACR_EL1, x0

    # Jump to kernel main
    bl kmain

    # In case kmain returns
    b .

# Setup the stacks in the BSS. I like my stacks page-aligned pls
# TODO: Should probably make stacks bigger/flexible size
.comm __el1_stack_limit, 4096, 4096

# Setup some boot time variables in the BSS
.comm kernel_physical_start, 8, 8
