#ifndef __VM_MAP_H__
#define __VM_MAP_H__

#include <sys/types.h>

#include <kernel/pmap.h>
#include <kernel/vm_object.h>

// A virtual memory mapping represents a contiguous range of virtual address space with the same
// protections and attributes. Mappings are part of a single map and organized in red/black tree
// the root of which is contained in the map. Mappings are linked a virtual memory object which
// provides the data for the mapped virtual address range. Mappings may not actually have a physical
// address range associated with it; physical pages are linked to mappings on demand
typedef struct vm_mapping {
    struct vm_mapping *left, *right; // Children vm_mappings
    vaddr_t vstart, vend;            // The start and end addresses of this virtual memory region 
    vm_prot_t prot;                  // The region's attributes 
    vm_object_t object;              // The VM object that this vregion is mapping
} vm_mapping_t;

// A virtual memory map represents the entire virtual address space of a process. The map contains
// multiple mappings each of which represents a valid virtual address range that a process can 
// access.
typedef struct { 
    spinlock_t lock;         // Multiple readers, single writer lock 
    pmap_t *pmap;            // The pmap associated with this vmap 
    vm_mapping_t *root;      // A list of contiguous virtual memory regions associated with this virtual memory space 
    size_t size;             // Total size of the virtual address space defined by this map
    vaddr_t start, end;      // The start and end virtual addresses of the entire virtual space definied by this map
} vm_map_t;

// Declare the kernel's vmap
extern vm_map_t kernel_vmap;

// Initialize the VMM system and initializes the kernel's vm_map
void vm_map_init(void);

// Returns a reference to the kernel's vm_map
#define vm_map_kernel() (&(kernel_vmap))

#endif // __VM_MAP_H__
