# Memory and instruction barriers, these can be called in C code
.text

# Data Memory Barrier
# Use this to make sure demand memory accesses (i.e. loads & stores) before the instruction complete before
# memory accesses after the instruction execute
.global arch_barrier_dmb
.align 2
arch_barrier_dmb:
    dmb sy
    ret lr

# Data Synchronization Barrier
# Use this when you need to ensure that all memory accesses complete before
# execution of the code resumes (this includes MMU, cache maintenance accesses)
.global arch_barrier_dsb
.align 2
arch_barrier_dsb:
    dsb sy
    ret lr

# Instruction Synchronization Barrier
# Flushes the instruction pipeline in the processor
# Use this whenever instructions after the ISB depend on instructions before the ISB to have retired
.global arch_barrier_isb
.align 2
arch_barrier_isb:
    isb sy
    ret lr
