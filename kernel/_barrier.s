# Memory and instruction barriers, these can be called in C code
.text
.code 32

# Data Memory Barrier
# Use this to make sure demand memory accesses (i.e. loads & stores) before the instruction complete before
# memory accesses after the instruction execute
.global barrier_dmb
.type barrier_dmb, %function
.align 2
barrier_dmb:
    dmb

    bx lr

# Data Synchronization Barrier
# Use this when you need to ensure that all memory accesses complete before
# execution of the code resumes (this includes MMU, cache maintenance accesses)
.global barrier_dsb
.type barrier_dsb, %function
.align 2
barrier_dsb:
        dsb

    bx lr

# Instruction Synchronization Barrier
# Flushes the instruction pipeline in the processor
# Use this whenever instructions after the ISB depend on instructions before the ISB to have retired
.global barrier_isb
.type barrier_isb, %function
.align 2
barrier_isb:
        isb

    bx lr
