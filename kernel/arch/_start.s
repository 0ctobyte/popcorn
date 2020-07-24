/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

.section ".text.start"

.global _start
.align 2
_start:
    mrs x1, MPIDR_EL1
    and x1, x1, #0xff
    cbnz x1, _wfe_loop

    # Mask exceptions and interrupts for now
    msr DAIFSet, #0xf

    # Save the pointer to the device tree
    mov x13, x0

    # Check if we are running in EL2 and switch to EL1
    mrs x0, CurrentEL
    cmp x0, #4
    beq _not_el2

    # Enable AARCH64 mode in EL1; Disable general exceptions trap
    mov x0, #1
    mov x1, xzr
    bfi x1, x0, #31, #1
    msr HCR_EL2, x1
    # Switch to EL1
    mov x0, #0x3c5
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

    # Get the start of the kernel's physical address space
    adr x11, _start

    # Calculate the end of the kernel's physical address space
    ldr x12, =__kernel_physical_end
    add x12, x12, x11

    # Set the el1 stack to after the kernel image in memory (for now it's a physical address until we enable the MMU)
    add x12, x12, #4096
    and x12, x12, #-16
    mov sp, x12

    # Create a the first frame record, this should be 0
    mov fp, xzr
    mov lr, xzr
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Flush and invalidate the dcache and icache and TLB
    bl arch_dcache_flush_all
    ic ialluis
    dsb sy
    isb sy
    tlbi vmalle1is
    dsb sy
    isb sy

    # Enable the icache, dcache
    mrs x0, SCTLR_EL1
    mov x1, #0x1004
    orr x0, x0, x1
    msr SCTLR_EL1, x0

    # Clear the BSS
    ldr x0, =__kernel_physical_bss_start
    ldr x1, =__kernel_physical_bss_end
    sub x1, x1, x0
    add x0, x0, x11
    bl arch_fast_zero

    # Store the physical base address of where the kernel was loaded in memory
    adr x0, kernel_physical_start
    str x11, [x0]

    # Store the end of the kernel physical address space
    adr x0, kernel_physical_end
    str x12, [x0]

    # Finally copy the FDT to the end of the kernel's physical address space
    # _copy_fdt returns the total size of the FDT header which we use to bump up kernel_physical_end
    adr x0, fdt_header
    str x12, [x0]
    mov x0, x12
    mov x1, x13
    bl _copy_fdt
    adr x2, kernel_physical_end
    ldr x1, [x2]
    add x0, x0, x1
    str x0, [x2]

    # Jump to kernel main
    bl kmain

    # In case kmain returns
    ldp fp, lr, [sp], #16

_wfe_loop:
    wfe
    b _wfe_loop

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

.comm kernel_physical_start, 8, 8
.comm kernel_physical_end, 8, 8
.comm fdt_header, 8, 8
