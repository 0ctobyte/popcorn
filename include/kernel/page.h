#ifndef __PAGE_H__
#define __PAGE_H__

#include <kernel/mm.h>
#include <kernel/spinlock.h>

typedef struct page_s {
    unsigned int wired_count;           // How many virtual maps have wired this page
    struct page_status_s {              // Status bits indicating the state of this page
        unsigned int is_referenced:1;   // Has this page been referenced recently
        unsigned int is_dirty:1;        // Has this page been modified
        unsigned int is_active:1;       // Is this page being used i.e. mapped in some virtual map
    } status;
    struct page_s *next_buddy;          // Next free buddy. Used by the buddy page allocater
    struct page_s *next_resident;       // List of resident pages. This is used by the object that has this page mapped
} page_t;

void page_init(paddr_t page_array_pa, paddr_t mem_base_addr, size_t mem_size, size_t page_size);

// Allocate or free a contiguous range of pages
page_t* page_alloc_contiguous(size_t num_pages);
void page_free_contiguous(page_t *pages, size_t num_pages);

// Allocate or free one page
page_t* page_alloc(void);
void page_free(page_t *page);

// Convert a page to a physical address and vice versa
paddr_t page_get_pa(page_t *page);
page_t* page_from_pa(paddr_t pa);

// Reserve pages that have been allocated but not through page_alloc*. This should be used by the virtual memory system
// during boot time initialization to tell the page system what pages are being used by the kernel
page_t* page_reserve_pa(paddr_t pa);

#endif // __PAGE_H__
