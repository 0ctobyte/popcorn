#ifndef __PMM_H__
#define __PMM_H__

#include <sys/types.h>

typedef uintptr_t address_t;

/* pmm_init needs to be FIXED */
void pmm_init();

/*
 * Finds a free page frame and returns the physical address to the beginning of
 * the frame
 * Returns UINTPTR_MAX if no free page frame is found
 */
address_t pmm_alloc();

/*
 * Finds a contiguous range of free page frames and returns the physical
 * address to first page frame in the range
 * num_frames [in] - # of page frames to allocate
 * alignment [in] - # of low order bits that should be zero in the physical
 * address of the first page frame in the range. In other words, the first 
 * page frame in the contiguous range will be aligned to 2^(alignment) bytes.
 * If it still doesn't make sense; the physical address of the first page frame
 * in the contiguous range will be a multiple of 2^(alignment).
 * Returns UINTPTR_MAX if no such contiguous range can be found
 */
address_t pmm_alloc_contiguous(size_t num_frames, uint32_t alignment);

/*
 * Frees the allocated page frame.
 */
void pmm_free(address_t);

#endif // __PMM_H__

