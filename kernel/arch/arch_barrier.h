#ifndef _ARCH_BARRIER_H_
#define _ARCH_BARRIER_H_

// Data Memory Barrier
// Use this to make sure demand memory accesses (i.e. loads & stores) before the instruction complete before
// memory accesses after the instruction execute
void arch_barrier_dmb(void);

// Data Synchronization Barrier
// Use this when you need to ensure that all memory accesses complete before
// execution of the code resumes (this includes MMU, cache maintenance accesses)
void arch_barrier_dsb(void);

// Instruction Synchronization Barrier
// Flushes the instruction pipeline in the processor
// Use this whenever instructions after the ISB depend on instructions before the ISB to have retired
void arch_barrier_isb(void);

#endif // _ARCH_BARRIER_H_
