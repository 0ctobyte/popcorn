#include <kernel/pmap.h>

typedef enum {
  VM_FILE,
  VM_ZEROFILL,
  VM_DEVICE
} vobject_type_t;

typedef struct vpage {
  // The starting virtual address of the page
  vaddr_t vaddr;

  // Values indicating the status of this page
  // TODO: Not sure what this means yet
  uint32_t status;
} vpage_t;

typedef struct vobject {
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
  vaddr_t stack_start, stack_end;
} vmap_t;


// static vmap_t kernel_vmap;

void _vmap_kernel_init() {
}

void vmm_init() {
}


