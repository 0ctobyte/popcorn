.text

.global tlb_invalidate_all
.align 2
tlb_invalidate_all:
    tlbi vmalle1is
    dsb sy
    isb sy
    ret lr

# x0 - va_start
# x1 - asid
.global tlb_invalidate_va
.align 2
tlb_invalidate_va:
    lsr x0, x0, #12
    bfi x0, xzr, #48, #16
    bfi x0, x1, #48, #16
    tlbi vale1is, x0
    dsb sy
    isb sy
    ret lr

# x0 - asid
.global tlb_invalidate_asid
.align 2
tlb_invalidate_asid:
    mov x1, xzr
    bfi x1, x0, #48, #16
    tlbi aside1is, x1
    dsb sy
    isb sy
    ret lr

.global mmu_is_4kb_granule_supported
.align 2
mmu_is_4kb_granule_supported:
    mrs x0, ID_AA64MMFR0_EL1
    ubfx x0, x0, #28, #4
    cmp x0, #0x0
    csetm x0, eq
    ret lr

.global mmu_is_16kb_granule_supported
.align 2
mmu_is_16kb_granule_supported:
    mrs x0, ID_AA64MMFR0_EL1
    ubfx x0, x0, #20, #4
    cmp x0, #0x1
    csetm x0, eq
    ret lr

.global mmu_is_64kb_granule_supported
.align 2
mmu_is_64kb_granule_supported:
    mrs x0, ID_AA64MMFR0_EL1
    ubfx x0, x0, #24, #4
    cmp x0, #0x0
    csetm x0, eq
    ret lr

.global mmu_is_enabled
.align 2
mmu_is_enabled:
    mrs x0, SCTLR_EL1
    tst x0, #1
    csetm x0, ne
    ret lr

// Enable the MMU
// x0 - TTBR0
// x1 - TTBR1
// x2 - MAIR
// x3 - Page Size
.global mmu_enable
.align 2
mmu_enable:
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp lr, fp, [sp, #-16]!
    mov fp, sp

    # Check if mmu is enabled and exit if so
    bl mmu_is_enabled
    cbnz x0, mmu_enable_exit
    ldp x2, x3, [sp, #32]
    ldp x0, x1, [sp, #48]

    # Set up the TCR
    mov x4, xzr
    # Set T0SZ and T1SZ to 16 to cover the whole VA space in both regions
    mov x5, #0x10
    bfi x4, x5, #0, #6
    bfi x4, x5, #16, #6
    # Set IRGN and ORGN to normal write-back, read-allocate, write-allocate for TTBR0 & TTBR1
    mov x5, #0x11
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
    bl tlb_invalidate_all

mmu_enable_exit:
    ldp lr, fp, [sp], #16
    add sp, sp, #48
    ret lr

# x0 - pa_base
# x1 - va_base
.global mmu_kernel_longjmp
.align 2
mmu_kernel_longjmp:
    sub sp, sp, x0
    add sp, sp, x1

    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # Update stack frame pointers
    ldp x2, x3, [fp]
mmu_kernel_longjmp_loop:
    sub x2, x2, x0
    add x2, x2, x1
    sub x3, x3, x0
    add x3, x3, x1
    stp x2, x3, [fp]
    mov fp, x2
    ldp x2, x3, [fp]
    cbnz x2, mmu_kernel_longjmp_loop

    mov fp, sp
    adr x2, mmu_kernel_longjmp_done
    sub x2, x2, x0
    add x2, x2, x1
    br x2

mmu_kernel_longjmp_done:
    ldp fp, lr, [sp], #16
    ret lr

.global mmu_get_ttbr0
.align 2
mmu_get_ttbr0:
    mrs x0, TTBR0_EL1
    ret lr

# x0 - Translation Table Base address
# x1 - ASID
.global mmu_set_ttbr0
.align 2
mmu_set_ttbr0:
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

.global mmu_clear_ttbr0
.align 2
mmu_clear_ttbr0:
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

# x0 - VA to translate
.global mmu_translate_va
.align 2
mmu_translate_va:
    at S1E1R, x0
    mrs x1, PAR_EL1
    ubfx x0, x1, #12, #36
    lsl x0, x0, #12
    mvn x2, xzr
    tst x1, #1
    csel x0, x0, x2, eq
    ret lr
