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
