.text

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

    # Flush the dcache and invalidate the icache
    stp x4, x5, [sp, #16]
    bl dcache_flush_all
    bl icache_invalidate_all
    ldp x4, x5, [sp, #16]
    ldp x2, x3, [sp, #32]
    ldp x0, x1, [sp, #48]

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
