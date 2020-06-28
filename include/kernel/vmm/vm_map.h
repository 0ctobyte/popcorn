#ifndef __VM_MAP_H__
#define __VM_MAP_H__

#include <sys/types.h>
#include <kernel/spinlock.h>
#include <kernel/rbtree.h>
#include <kernel/list.h>
#include <kernel/arch/atomic.h>
#include <kernel/arch/pmap.h>
#include <kernel/vmm/vmm.h>
#include <kernel/vmm/vm_object.h>

// A virtual memory mapping represents a contiguous range of virtual address space with the same
// protections and attributes. Mappings are part of a single map and organized in a red/black tree
// Mappings are linked to a virtual memory object which provides the data for the mapped virtual address range.
// Mappings may not actually have a physical address range associated with it; physical pages are linked to mappings on demand
typedef struct vm_mapping {
    list_node_t   ll_node;  // Linked list linkage
    rbtree_node_t rb_node;  // Red/black tree linkage of mappings based on starting virtual address
    rbtree_node_t rb_hole;  // Red/black tree linkage of mappings based on the size of the virtual address space hole after the mapping
    size_t hole_size;       // Size of the virtual address space hole after this mapping
    vaddr_t vstart, vend;   // The start and end addresses of this virtual memory region
    vm_prot_t prot;         // The region's attributes
    vm_object_t *object;    // The VM object that this vregion is mapping
    vm_offset_t offset;     // The offset into the object that the mapping starts from
    bool wired;             // Is this a wired mapping?
} vm_mapping_t;

// A virtual memory map represents the entire virtual address space of a process. The map contains
// multiple mappings each of which represents a valid virtual address range that a process can access.
typedef struct {
    spinlock_t lock;         // Multiple readers, single writer lock
    pmap_t *pmap;            // The pmap associated with this vmap
    list_t ll_mappings;      // Linked list of mappings sorted by virtual address. Used to iterate through mappings
    rbtree_t rb_mappings;    // All contiguous virtual memory regions associated with this virtual memory space rooted in a red/black tree
    rbtree_t rb_holes;       // A red/black tree of mappings ordered by the holes in the virtual address space afer the mapping
    vaddr_t start, end;      // The start and end virtual addresses of the entire possible virtual space definied by this map
    size_t size;             // Total size of the current virtual address space defined by this map
    atomic_t refcnt;         // Reference count
} vm_map_t;

// Declare the kernel's vmap
extern vm_map_t kernel_vmap;

// Initialize the vm_map system
void vm_map_init(void);

// Creates an empty map
vm_map_t* vm_map_create(pmap_t *pmap, vaddr_t vmin, vaddr_t vmax);

// Drops a reference to the given map. If the reference count drops to zero all resources for this map will be released
void vm_map_destroy(vm_map_t *vmap);

// Increases the reference count on the given map
void vm_map_reference(vm_map_t *vmap);

// Enter a mapping of the given size into object starting at offset with the specified protection and at the given virtual address
// This routine will check to make sure the given virtual address and size can fit in the map since mapping entries cannot overlap
// Return an error code if failed
kresult_t vm_map_enter_at(vm_map_t *vmap, vaddr_t vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot);

// Enter a mapping of the given size into object starting at offset with the specified protection
// This will find the first available virtual address range that can fit this mapping and return the address in vaddr
// Return an error code if failed
kresult_t vm_map_enter(vm_map_t *vmap, vaddr_t *vaddr, size_t size, vm_object_t *object, vm_offset_t offset, vm_prot_t prot);

// Remove the given virtual address range from the map
kresult_t vm_map_remove(vm_map_t *vmap, vaddr_t start, vaddr_t end);

// Sets new protection attributes for the given virtual address range in the specified map
kresult_t vm_map_protect(vm_map_t *vmap, vaddr_t start, vaddr_t end, vm_prot_t new_prot);

// Wires a range of virtual memory. This routine will allocate and map pages in the pmap
kresult_t vm_map_wire(vm_map_t *vmap, vaddr_t start, vaddr_t end);

// Unwires a range of virtual memory. This will not free pages already mapped
kresult_t vm_map_unwire(vm_map_t *vmap, vaddr_t start, vaddr_t end);

// Given the map, virtual address and fault (i.e. access) type, returns the object, offset and protection of the virtual address
kresult_t vm_map_lookup(vm_map_t *vmap, vaddr_t vaddr, vm_prot_t fault_type, vm_object_t *object, vm_offset_t *offset, vm_prot_t *prot);

// Returns a reference to the kernel's vm_map
#define vm_map_kernel() (&(kernel_vmap))

#endif // __VM_MAP_H__
