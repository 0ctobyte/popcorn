.text

.global tlb_invalidate_all
.align 2
tlb_invalidate_all:
    tlbi vmalle1is
    dsb sy
    isb sy
    ret lr

# x0 - va_start
.global tlb_invalidate
.align 2
tlb_invalidate:
    lsr x0, x0, #12
    tlbi vale1is, x0
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

# This algorithm flushes every data cache in the system by set/way:
# For each cache level, check if there is a data cache using the CLIDR_EL1 register. If not we're done.
# If there is a data cache, set the CSSELR_EL1 to that level and read the CCSIDR_EL1 register
# Get the numsets and associativity from that register and Calculate bit shifts for the set and way
# Then for each way and for each set execute the DC CISW instruction
.global dcache_flush_all
.align 2
dcache_flush_all:
    stp x8, x9, [sp, #-16]!
    stp fp, lr, [sp, #-16]!
    mov fp, sp

    mrs x0, CLIDR_EL1

    # Cache level
    mov x1, xzr

dcache_flush_all_level_loop:
    # Extract the cache type for the given level. We only care about data caches, the encoding:
    # 0b000 - No cache
    # 0b001 - Instruction cache only
    # 0b010 - Data cache only
    # 0b011 - Separate data and instruction cache
    # 0b100 - Unified cache
    mov x2, #3
    mul x2, x1, x2
    lsr x2, x0, x2
    ands xzr, x2, #7
    beq dcache_flush_all_done       // Stop if there are no caches at the current level
    ands xzr, x2, #6
    beq dcache_flush_all_level_loop // Skip to next level if there is no data/unified cache at the current level

    # Select the cache level with the Cache Size Selection register
    # x2 holds the level# << 1
    lsl x2, x1, #1
    msr CSSELR_EL1, x2
    isb sy

    # Get the # of sets, associativity and line length from the Current Cache Size ID register
    # FIXME: The bit fields of this register are different if ARMv8.3-CCIDX is implemented!
    mrs x3, CCSIDR_EL1
    ubfx x5, x3, #13, #15  // NUMSETS-1
    ubfx x4, x3, #3, #10   // ASSOCIATIVITY-1
    ubfx x3, x3, #0, #3    // log2(LINELEN)-4
    add x3, x3, #4         // log2(LINELEN)

    # Figure out the LSB of the way bits used in the DC CISW instruction: 32-A where A=log2(ASSOCIATIVITY)
    clz x6, x4
    mov x7, #64
    subs x6, x7, x6
    cinc x6, x6, eq  // This is our log2(ASSOCIATIVITY). Increment if ASSOCIATIVITY=1
    mov x7, #32
    sub x6, x7, x6

dcache_flush_all_way_loop:
    mov x8, x5

dcache_flush_all_set_loop:
    # Set the Set, Way and level bits
    # LSB of the set bits used in the DC CISW instruction: L = log2(LINELEN)
    lsl x7, x4, x6
    orr x7, x7, x2
    lsl x9, x8, x3
    orr x7, x7, x9

    # Clean and invalidate by Set/Way
    DC CISW, x7

    # Keep looping for all sets
    subs x8, x8, #1
    bge dcache_flush_all_set_loop

    # Keep looping for all ways
    subs x4, x4, #1
    bge dcache_flush_all_way_loop

    # Keep looping for all levels
    # ARMv8 supports a maximum of 7 levels
    add x1, x1, #1
    cmp x1, #7
    bne dcache_flush_all_level_loop

    dsb sy
    isb sy

dcache_flush_all_done:
    ldp fp, lr, [sp], #16
    add sp, sp, #16
    ret lr
