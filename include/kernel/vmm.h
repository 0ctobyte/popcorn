#ifndef __VMM_H__
#define __VMM_H__

#include <sys/types.h>

#include <kernel/pmap.h> 

typedef enum {
  VM_FILE,
  VM_ZEROFILL,
  VM_DEVICE
} vobject_type_t;

typedef struct {
  // The starting virtual address of the page
  vaddr_t vaddr;
} vpage_t;

typedef struct {
  // The list of pages owned by this object
  vpage_t *pages;

  // The object type (anonymous or zero-fill, file, device)
  vobject_type_t type;
} vobject_t;

typedef struct vregion {
  // The start and end addresses of this virtual memory region
	vaddr_t start, end;

  // The region's attributes
  vm_prot_t prot; 

  // The memory object mapped into this virtual memory region
  vobject_t obj;

	struct vregion *next;
} vregion_t;

typedef struct {
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

// Returns a reference to the kernel's vmap
#define vmap_kernel() (&(kernel_vmap))

#endif // __VMM_H__

