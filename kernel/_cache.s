.global _tlb_invalidate_all
.align 2
_tlb_invalidate_all:
    tlbi alle1
    dsb sy
    isb sy
    ret lr

.global _icache_invalidate_all
.align 2
_icache_invalidate_all:
    ic iallu
    dsb sy
    isb sy
    ret lr

.global _dcache_clean_invalidate_all
.align 2
_dcache_clean_invalidate_all:
    # This algorithm flushes every data cache in the system by set/way:
    # For each cache level, check if there is a data cache using the CLIDR_EL1 register. If not we're done.
    # If there is a data cache, set the CSSELR_EL1 to that level and read the CCSIDR_EL1 register
    # Get the numsets and associativity from that register and Calculate bit shifts for the set and way
    # Then for way and for each set execute the DC CISW instruction

    ret lr
