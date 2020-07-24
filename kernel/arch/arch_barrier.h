/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _ARCH_BARRIER_H_
#define _ARCH_BARRIER_H_

// Data Memory Barrier
// Use this to make sure demand memory accesses (i.e. loads & stores) before the instruction complete before
// memory accesses after the instruction execute
#define arch_barrier_dmb() asm volatile ("dmb sy")

// Data Synchronization Barrier
// Use this when you need to ensure that all memory accesses complete before
// execution of the code resumes (this includes MMU, cache maintenance accesses)
#define arch_barrier_dsb() asm volatile ("dsb sy")

// Instruction Synchronization Barrier
// Flushes the instruction pipeline in the processor
// Use this whenever instructions after the ISB depend on instructions before the ISB to have retired
#define arch_barrier_isb() asm volatile ("isb sy")

#endif // _ARCH_BARRIER_H_
