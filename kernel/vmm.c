#include <kernel/mm.h>

typedef struct vpage {
  // The starting virtual address of the page
  vaddr_t vaddr;

  // Values indicating the status of this page
  bool used, anonymous;
} vpage_t;

typedef struct vregion {
  // The start and end addresses of this virtual memory region
	vaddr_t start, end;

  // List of virtual pages belonging to this virtual memory region 
  vpage_t *vpages;

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

