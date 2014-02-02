#ifndef __BARRIER_H__
#define __BARRIER_H__

// Data Memory Barrier
// Use this to make sure memory accesses before the instruction complete before
// memory accesses after the instruction execute
void barrier_dmb();

// Data Synchronization Barrier
// Ensures all instructions before this instruction complete
// Use this when you need to ensure that memory accesses complete before
// execution of the code resumes
void barrier_dsb();

// Instruction Synchronization Barrier
// Flushes the instruction pipeline in the processor
// Use this whenever instructions after the ISB are invalid/changed
void barrier_isb();

#endif // __BARRIER_H__

