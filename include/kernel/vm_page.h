#ifndef __VM_PAGE_H__
#define __VM_PAGE_H__

#include <kernel/vmm.h>
#include <kernel/vm_object.h>
#include <kernel/spinlock.h>
#include <lib/list.h>

typedef struct vm_object_s vm_object_t;

typedef struct vm_page_s {
    struct vm_page_status_s {           // Status bits indicating the state of this page
        unsigned int wired_count:16;    // How many virtual maps have wired this page
        unsigned int is_referenced:1;   // Has this page been referenced recently
        unsigned int is_dirty:1;        // Has this page been modified
        unsigned int is_active:1;       // Is this page being used i.e. mapped in some virtual map
    } status;
    list_node_t ll_node;                // Linked list of resident pages in an object or part of the buddy free list
    vm_object_t *object;                // VM object this page belongs to if any
    vm_offset_t offset;                 // Offset in that VM object that this page refers to
} vm_page_t;

// Will bootstrap the page allocation system using the space addressed by vm_page_array_addr with the given number of pages
void vm_page_bootstrap(paddr_t vm_page_array_addr);

// Second stage initialization after pmap has been initialized and kernel is running in virtual memory mode
void vm_page_init(void);

// Lookup a vm_page given a object and offset
vm_page_t* vm_page_lookup(vm_object_t *object, vm_offset_t offset);

// Allocate or free a contiguous range of pages
vm_page_t* vm_page_alloc_contiguous(size_t num_pages, vm_object_t *object, vm_offset_t offset);
void vm_page_free_contiguous(vm_page_t *pages, size_t num_pages);

// Allocate or free one page
vm_page_t* vm_page_alloc(vm_object_t *object, vm_offset_t offset);
void vm_page_free(vm_page_t *page);

// Increase/decrease the wire count on the page
void vm_page_wire(vm_page_t *page);
void vm_page_unwire(vm_page_t *page);

// Convert a page to a physical address and vice versa
paddr_t vm_page_to_pa(vm_page_t *page);
vm_page_t* vm_page_from_pa(paddr_t pa);

// Reserve pages that have been allocated but not through vm_page_alloc*. This should be used by the virtual memory system
// during boot time initialization to tell the page system what pages are being used by the kernel. These pages are wired
// These pages should be added to the provided kernel memory object
vm_page_t* vm_page_reserve_pa(paddr_t pa);

#endif // __VM_PAGE_H__
