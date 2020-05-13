#ifndef __BARRIER_H__
#define __BARRIER_H__

// Data Memory Barrier
// Use this to make sure demand memory accesses (i.e. loads & stores) before the instruction complete before
// memory accesses after the instruction execute
void barrier_dmb(void);

// Data Synchronization Barrier
// Use this when you need to ensure that all memory accesses complete before
// execution of the code resumes (this includes MMU, cache maintenance accesses)
void barrier_dsb(void);

// Instruction Synchronization Barrier
// Flushes the instruction pipeline in the processor
// Use this whenever instructions after the ISB depend on instructions before the ISB to have retired
void barrier_isb(void);

#endif // __BARRIER_H__

