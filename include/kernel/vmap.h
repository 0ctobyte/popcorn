#ifndef __VMM_H__
#define __VMM_H__

#include <sys/types.h>

#include <kernel/pmap.h>
#include <kernel/page.h>
#include <kernel/spinlock.h>

typedef struct {
    page_t *resident_pages; // List of resident pages for this object
    atomic_t refcnt;        // How many VM regions are referencing this object
} vobject_t;

typedef struct vregion {
    struct vregion *next; // List of vregions 
    vaddr_t vstart, vend; // The start and end addresses of this virtual memory region 
    vprot_t prot;         // The region's attributes 
    vobject_t object;     // The VM object that this vregion is mapping
} vregion_t;

typedef struct { 
    spinlock_t lock;              // Multiple readers, single writer lock 
    pmap_t *pmap;                 // The pmap associated with this vmap 
    vregion_t *regions;           // A list of contiguous virtual memory regions associated with this virtual memory space 
    size_t size;                  // Total size of the virtual address space defined by this map
    vaddr_t start, end;           // The start and end virtual addresses of the entire virtual space definied by this map
    vaddr_t text_start, text_end; // Code start and end virtual addresses
    vaddr_t data_start, data_end; // Data start and end virtual addresses
    vaddr_t heap_start, heap_end; // Heap start and end virtual addresses
    vaddr_t stack_start;          // Stack starting address
} vmap_t;

// Declare the kernel's vmap
extern vmap_t kernel_vmap;

// Initialize the VMM system and initializes the kernel's vmap
void vmap_init(void);

// Returns a reference to the kernel's vmap
#define vmap_kernel() (&(kernel_vmap))

#endif // __VMM_H__

