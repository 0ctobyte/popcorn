#ifndef __VM_PAGE_H__
#define __VM_PAGE_H__

#include <kernel/vmm.h>
#include <kernel/vm_object.h>
#include <kernel/spinlock.h>

typedef struct vm_object_s vm_object_t;

typedef struct vm_page_s {
    struct vm_page_status_s {           // Status bits indicating the state of this page
        unsigned int wired_count:16;    // How many virtual maps have wired this page
        unsigned int is_referenced:1;   // Has this page been referenced recently
        unsigned int is_dirty:1;        // Has this page been modified
        unsigned int is_active:1;       // Is this page being used i.e. mapped in some virtual map
    } status;
    struct vm_page_s *next_buddy;       // Next free buddy. Used by the buddy page allocater
    struct vm_page_s *prev_resident;    // List of resident pages. This is used by the object that has this page mapped
    struct vm_page_s *next_resident;    // List of resident pages. This is used by the object that has this page mapped
    vm_object_t *object;                // VM object this page belongs to if any
    vm_offset_t offset;                 // Offset in that VM object that this page refers to
} vm_page_t;

// Will initialize the page allocation system using the space addressed by vm_page_array_addr with the given number of pages
void vm_page_init(paddr_t vm_page_array_addr);

// Allocate or free a contiguous range of pages
vm_page_t* vm_page_alloc_contiguous(size_t num_pages, vm_object_t *object, vm_offset_t offset);
void vm_page_free_contiguous(vm_page_t *pages, size_t num_pages);

// Allocate or free one page
vm_page_t* vm_page_alloc(vm_object_t *object, vm_offset_t offset);
void vm_page_free(vm_page_t *page);

// Steal a page of memory not associated with an object
// FIXME Need this to allocate page tables but it doesn't seem like a great idea
vm_page_t* vm_page_steal(void);

// Convert a page to a physical address and vice versa
paddr_t vm_page_to_pa(vm_page_t *page);
vm_page_t* vm_page_from_pa(paddr_t pa);

// Reserve pages that have been allocated but not through vm_page_alloc*. This should be used by the virtual memory system
// during boot time initialization to tell the page system what pages are being used by the kernel. These pages are wired
// These pages should be added to the provided kernel memory object
vm_page_t* vm_page_reserve_pa(paddr_t pa);

// Relocate the page array to it's virtual address, this is called by the virtual memory subsystem in it's initialization process
// after it has enabled the MMU
void vm_page_relocate_array(vaddr_t va);

#endif // __VM_PAGE_H__
