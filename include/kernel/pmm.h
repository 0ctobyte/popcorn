#ifndef __PMM_H__
#define __PMM_H__

#include <sys/types.h>
#include <kernel/mm.h>

// Assume PAGESIZE is 4KB, the smallest ARMv8 page granule.
// pmap will have to allocate and free the right amount of 4KB pages when using larger page granules
#define PMM_PAGE_SIZE  (4096)
#define PMM_PAGE_SHIFT (12)

// Initializes the pmm
// NOTE: pmap_init should be called before this function
void pmm_init(uintptr_t mem_base_addr, size_t mem_size);

// Finds a free page page and returns the physical address to the beginning of the page in addr
// Returns false if it failed to find a free page
bool pmm_alloc(paddr_t *addr);

// Frees the allocated page page.
void pmm_free(paddr_t addr);

// Finds a contiguous range of free page pages and returns the physical address to first page in the range in addr
// Frames cannot be greater than #BITS per pagemap
// Returns false if it failed to find free pages
bool pmm_alloc_contiguous(paddr_t *addr, size_t pages);

// Finds a contiguous range of free page pages aligned to a multiple of 4KB. The multiple is specified by alignment
// Ex. so an alignment of 4 means 16KB alignmed
// Frames cannot be greater than #BITS per pagemap
// Returns false if it failed to find the required free pages
bool pmm_alloc_contiguous_aligned(paddr_t *addr, size_t pages, unsigned int alignment);

// Marks the page at the specified physical address as used
// This should be used to mark pages that the kernel uses on startup
void pmm_reserve(paddr_t addr);

#endif // __PMM_H__

