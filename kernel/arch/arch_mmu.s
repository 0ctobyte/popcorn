/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

.text

# Enable the MMU
# x0 - TTBR0
# x1 - TTBR1
# x2 - MAIR
# x3 - Page Size
.global arch_mmu_enable
.align 2
arch_mmu_enable:
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp lr, fp, [sp, #-16]!
    mov fp, sp

    # Check if mmu is enabled and exit if so
    mrs x0, SCTLR_EL1
    tst x0, #1
    csetm x0, ne
    cbnz x0, _mmu_enable_exit
    ldp x2, x3, [sp, #32]
    ldp x0, x1, [sp, #48]

    # Set up the TCR
    mov x4, xzr
    # Set T0SZ and T1SZ to 16 to cover the whole VA space in both regions
    mov x5, #0x10
    bfi x4, x5, #0, #6
    bfi x4, x5, #16, #6
    # Set IRGN and ORGN to normal write-back, read-allocate, write-allocate for TTBR0 & TTBR1
    mov x5, #0x5
    bfi x4, x5, #8, #4
    bfi x4, x5, #24, #4
    # Shareability for TTBR0 & TTBR1 is inner shareable
    mov x5, #0x3
    bfi x4, x5, #12, #2
    bfi x4, x5, #28, #2
    # TG0 & TG1 page granule size
    mov x5, #1
    mov x6, #2
    cmp x3, #0x1000
    csel x5, xzr, x5, eq
    cmp x3, #0x4000
    csel x5, x6, x5, eq
    bfi x4, x5, #14, #2
    mov x5, #1
    mov x6, #2
    cmp x3, #0x4000
    csel x5, xzr, x5, eq
    cmp x3, #0x10000
    csel x5, x6, x5, eq
    add x5, x5, #1
    bfi x4, x5, #30, #2
    # Program the intermedia Physical address size to that in ID_AA64MMFR0_EL1.PARange
    mrs x5, ID_AA64MMFR0_EL1
    bfi x4, x5, #32, #3

    # Disable the icache and dcache
    mrs x5, SCTLR_EL1
    mov x6, #0x1004
    bic x5, x5, x6
    msr SCTLR_EL1, x5
    isb sy

    # Program the MMU registers
    msr TTBR0_EL1, x0
    msr TTBR1_EL1, x1
    msr MAIR_EL1, x2
    msr TCR_EL1, x4
    isb sy

    # Now enable the icache, dcache and MMU
    mov x6, #0x1005
    orr x5, x5, x6
    msr SCTLR_EL1, x5
    isb sy

    # Invalidate the TLB
    tlbi vmalle1is
    dsb sy
    isb sy

_mmu_enable_exit:
    ldp lr, fp, [sp], #16
    add sp, sp, #48
    ret lr

# x0 - pa_base
# x1 - va_base
.global arch_mmu_kernel_longjmp
.align 2
arch_mmu_kernel_longjmp:
    sub sp, sp, x0
    add sp, sp, x1

    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Update stack frame pointers
    ldp x2, x3, [fp]
_mmu_kernel_longjmp_loop:
    sub x2, x2, x0
    add x2, x2, x1
    sub x3, x3, x0
    add x3, x3, x1
    stp x2, x3, [fp]
    mov fp, x2
    ldp x2, x3, [fp]
    cbnz x2, _mmu_kernel_longjmp_loop

    mov fp, sp
    adr x2, _mmu_kernel_longjmp_done
    sub x2, x2, x0
    add x2, x2, x1
    br x2

_mmu_kernel_longjmp_done:
    ldp fp, lr, [sp], #16
    ret lr

# x0 - Translation Table Base address
# x1 - ASID
.global arch_mmu_set_ttbr0
.align 2
arch_mmu_set_ttbr0:
    mrs x2, DAIF
    orr x3, x2, #0xC0
    msr DAIF, x3

    # Disable TTBR0 and clear it
    dsb sy
    isb sy
    mov x3, #0x80
    mrs x4, TCR_EL1
    orr x4, x4, x3
    msr TCR_EL1, x4
    isb sy

    # Set new TTBR0 and ASID
    orr x0, x0, x1, lsl #48
    msr TTBR0_EL1, x0
    isb sy

    # Re-enable TTBR0
    mrs x4, TCR_EL1
    bic x4, x4, x3
    msr TCR_EL1, x4
    isb sy

    msr DAIF, x2
    isb sy

    ret lr

.global arch_mmu_clear_ttbr0
.align 2
arch_mmu_clear_ttbr0:
    mrs x2, DAIF
    orr x3, x2, #0xC0
    msr DAIF, x3

    # Disable TTBR0 and clear it
    mov x1, #0x80
    mrs x0, TCR_EL1
    orr x0, x0, x1
    msr TCR_EL1, x0
    isb sy

    msr TTBR0_EL1, xzr
    isb sy

    # Re-enable TTBR0
    mrs x0, TCR_EL1
    bic x0, x0, x1
    msr TCR_EL1, x0
    isb sy

    msr DAIF, x2
    isb sy

    ret lr
