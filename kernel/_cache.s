.global tlb_invalidate_all
.align 2
tlb_invalidate_all:
    tlbi vmalle1is
    dsb sy
    isb sy
    ret lr

.global icache_invalidate_all
.align 2
icache_invalidate_all:
    ic ialluis
    dsb sy
    isb sy
    ret lr

.global dcache_flush_all
.align 2
dcache_flush_all:
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    # This algorithm flushes every data cache in the system by set/way:
    # For each cache level, check if there is a data cache using the CLIDR_EL1 register. If not we're done.
    # If there is a data cache, set the CSSELR_EL1 to that level and read the CCSIDR_EL1 register
    # Get the numsets and associativity from that register and Calculate bit shifts for the set and way
    # Then for way and for each set execute the DC CISW instruction

    ldp fp, lr, [sp], #16
    ret lr
