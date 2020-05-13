#ifndef __PMM_H__
#define __PMM_H__

#include <sys/types.h>
#include <kernel/mm.h>

// Initializes the pmm
// NOTE: pmap_init should be called before this function
void pmm_init(void);

// Finds a free page frame and returns the physical address to the beginning of
// the frame
// Returns UINTPTR_MAX if no free page frame is found
paddr_t pmm_alloc(void);

// Frees the allocated page frame.
void pmm_free(paddr_t);

// Finds a contiguous range of free page frames and returns the physical
// address to first page frame in the range
// frames [in] - # of page frames to allocate, cannot be greater than 32
paddr_t pmm_alloc_contiguous(size_t frames);

// Marks the page at the specified physical address as used
// This should be used to mark pages that the kernel uses on startup
void pmm_reserve(paddr_t);

#endif // __PMM_H__

