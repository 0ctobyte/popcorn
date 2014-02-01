#ifndef __PMM_H__
#define __PMM_H__

#include <sys/types.h>

#define PAGESIZE (0x1000) // TODO: TEMPORARY
#define IS_PAGE_ALIGNED(B) (((B) & (PAGESIZE - 1)) == 0)

#ifndef __paddr_t
#define __paddr_t
typedef uintptr_t paddr_t;
#endif // __paddr_t

/* pmm_init needs to be FIXED */
void pmm_init();

/*
 * Finds a free page frame and returns the physical address to the beginning of
 * the frame
 * Returns UINTPTR_MAX if no free page frame is found
 */
paddr_t pmm_alloc();

/*
 * Frees the allocated page frame.
 */
void pmm_free(paddr_t);

/*
 * Finds a contiguous range of free page frames and returns the physical
 * address to first page frame in the range
 * frames [in] - # of page frames to allocate, cannot be greater than 32
 */
paddr_t pmm_alloc_contiguous(size_t frames);

#endif // __PMM_H__

