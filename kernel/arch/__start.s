.text

.global _start
.align 2
_start:
    # Mask exceptions and interrupts for now
    msr DAIFSet, #0xf

    # Save the pointer to the device tree
    adr x2, fdt_header
    str x0, [x2]

    # Check if we are running in EL2 and switch to EL1
    mrs x0, CurrentEL
    cmp x0, #4
    beq _not_el2
    # Enable AARCH64 mode in EL1
    mov x0, #1
    mrs x1, HCR_EL2
    bfi x1, x0, #31, #1
    msr HCR_EL2, x1
    # Switch to EL1
    mov x0, #0x3c4
    msr SPSR_EL2, x0
    adr x2, _not_el2
    msr ELR_EL2, x2
    eret

_not_el2:
    # Disable the MMU
    mrs x0, SCTLR_EL1
    mov x1, #1
    bic x0, x0, x1
    msr SCTLR_EL1, x0

    # Enable floating point & SIMD
    mrs x0, CPACR_EL1
    mov x1, #0x3
    bfi x0, x1, #20, #2
    msr CPACR_EL1, x0

    adr x11, _start

    # Set the el1 stack (for now it's a physical address until we enable the MMU)
    adr x0, __el1_stack_limit+8192
    mov sp, x0

    # Create a the first frame record, this should be 0
    mov fp, xzr
    mov lr, xzr
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Flush and invalidate the dcache and icache and TLB
    bl arch_dcache_flush_all
    bl arch_icache_invalidate_all
    bl arch_tlb_invalidate_all

    # Enable the icache, dcache
    mrs x0, SCTLR_EL1
    mov x1, #0x1004
    orr x0, x0, x1
    msr SCTLR_EL1, x0

    # Store the physical base address of where the kernel was loaded in memory
    adr x0, kernel_physical_start
    str x11, [x0]

    # Store the end of the kernel's physical address space
    ldr x0, =__kernel_physical_end
    add x0, x0, x11
    adr x1, kernel_physical_end
    str x0, [x1]

    # Finally copy the FDT to the end of the kernel's physical address space
    # _copy_fdt returns the total size of the FDT header which we use to bump up kernel_physical_end
    adr x1, fdt_header
    ldr x1, [x1]
    bl _copy_fdt
    adr x2, kernel_physical_end
    ldr x1, [x2]
    add x0, x0, x1
    str x0, [x2]

    # Jump to kernel main
    bl kmain

    # In case kmain returns
    ldp fp, lr, [sp], #16
    b .

# x0 - destination
# x1 - fdt_header pointer
.align 2
_copy_fdt:
    # x0 is the total size of the FDT
    mov x2, x0
    mov x0, xzr

    # Check for the magic value. All FDT values are big-endian so we need to use REV
    movk w4, #0xd00d, lsl #16
    movk w4, #0xfeed
    ldr w3, [x1]
    rev w3, w3
    cmp w3, w4
    bne _copy_fdt_done

    # Get the total size of the FDT
    ldr w3, [x1, #4]
    rev w3, w3
    mov w0, w3
_copy_fdt_loop:
    ldr w4, [x1], #4
    str w4, [x2], #4
    subs w3, w3, #1
    cbnz w3, _copy_fdt_loop

_copy_fdt_done:
    ret lr

# Setup the boot time stack in the BSS
.comm __el1_stack_limit, 8192, 8

.comm kernel_physical_start, 8, 8
.comm kernel_physical_end, 8, 8
.comm fdt_header, 8, 8
