#ifndef __VMM_H__
#define __VMM_H__

#include <sys/types.h>

#include <kernel/pmap.h> 
#include <kernel/spinlock.h>

typedef struct {
  // The starting virtual address of the page
  vaddr_t vaddr;
} vpage_t;

// Opaque types
typedef struct vm_anon vm_anon_t;
typedef struct vm_amap vm_amap_t;

struct vm_anon {
  // The page associated with this anon
  vpage_t *page;

  // How many amaps pointing to this anon?
  uint32_t refcount;
};

struct vm_amap {
  // An array of possible anon references associated with this anonymous map 
  vm_anon_t **aslots;

  // How many arefs pointing to this amap
  uint32_t refcount;

  // The max number of 'slots' (or elements) in the anon array
  uint32_t maxslots;

  // The number of slots currently occupied
  uint32_t nslots;
};


typedef struct {
  // A pointer to an amap and the slot offset within the anon array of that amap where this memory region begins
  vm_amap_t *amap;
  uint32_t slotoff;
} vm_aref_t;

typedef struct vregion {
  // The start and end addresses of this virtual memory region
	vaddr_t vstart, vend;

  // The region's attributes
  vm_prot_t vm_prot;
  uint16_t needs_copy, copy_on_write;

  // The anonymous memory associated with this region
  vm_aref_t aref;

	struct vregion *next;
} vregion_t;

typedef struct {
  // Multiple readers, single writer lock
  spinlock_t lock;

  // The pmap associated with this vmap
  pmap_t *pmap;

  // A list of contiguous virtual memory regions associated with this virtual memory space
  vregion_t *regions;

  // Main regions start and end virtual addresses
  vaddr_t text_start, text_end;
  vaddr_t data_start, data_end;
  vaddr_t heap_start, heap_end;
  vaddr_t stack_start;
} vmap_t;

// Declare the kernel's vmap
extern vmap_t kernel_vmap;

// Initialize the VMM system and initializes the kernel's vmap
void vmm_init();

// Find the vregion that encompasses the given address
vregion_t* vmm_region_lookup(vmap_t *vmap, vaddr_t va);

// Extends by size bytes (rounded to a page) the kernel heap region specified
// Returns the new (total) size of the region
size_t vmm_km_heap_extend(vregion_t *region, size_t size);

// Returns a reference to the kernel's vmap
#define vmap_kernel() (&(kernel_vmap))

#endif // __VMM_H__

