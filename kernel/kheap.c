#include <kernel/kheap.h>
#include <kernel/vmm.h>

static vaddr_t heap_start, heap_size;

void kheap_init() {
  heap_start = vmap_kernel()->heap_start;
  heap_size = PAGESIZE*5;
  vregion_t *region = vmm_region_lookup(vmap_kernel(), heap_start);
  heap_size = vmm_km_heap_extend(region, heap_size);
}

