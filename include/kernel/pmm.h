#ifndef __PMM_H__
#define __PMM_H__

#include <sys/types.h>
#include <kernel/mm.h>

// Returns the maximum amount of memory needed to allocate all pagemaps
size_t pmm_get_size_requirement(size_t mem_size, size_t page_size);

// Set the pagemaps location in the virtual address space. This should be called after the MMU is enabled.
void pmm_set_va(vaddr_t va);

// Initializes the pmm. va is the virtual address where PMM will allocate the pagemaps since the heap may not be initialized when pmm_init is called
// Before the MMU is enabled pmm_init will be called with the physical address region to allocate it's data structures. pmm_set_va is called after the
// mmu is enabled to switch to using the virtual address space
void pmm_init(paddr_t pa_alloc, paddr_t mem_base_addr, size_t mem_size, size_t page_size);

// Finds a free page page and returns the physical address to the beginning of the page in addr
// Returns false if it failed to find a free page
bool pmm_alloc(paddr_t *addr);

// Frees the allocated page page.
void pmm_free(paddr_t addr);

// Finds a contiguous range of free page pages and returns the physical address to first page in the range in addr
// Frames cannot be greater than #BITS per pagemap
// Returns false if it failed to find free pages
bool pmm_alloc_contiguous(paddr_t *addr, size_t pages);

// Finds a contiguous range of free page pages aligned to a multiple of page_size. The multiple is specified by alignment
// Ex. so an alignment of 4 means 16KB alignmed
// Frames cannot be greater than #BITS per pagemap
// Returns false if it failed to find the required free pages
bool pmm_alloc_contiguous_aligned(paddr_t *addr, size_t pages, unsigned int alignment);

// Marks the page at the specified physical address as used
// This should be used to mark pages that the kernel uses on startup
void pmm_reserve(paddr_t addr);

#endif // __PMM_H__

