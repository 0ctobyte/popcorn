#include <kernel/vmm.h>
#include <kernel/kassert.h>
#include <kernel/pmm.h>
#include <kernel/kheap.h>

#include <string.h>

#define IS_KERNEL_REGION(region) (((region) == &vmap_kernel()->regions[0]) ? true : (((region) == &vmap_kernel()->regions[1]) ? true : (((region) == &vmap_kernel()->regions[2]) ? true : (((region) == &vmap_kernel()->regions[3]) ? true : false))))

// Linker symbols
extern uintptr_t __text_virtual_start;
extern uintptr_t __text_virtual_end;
extern uintptr_t __data_virtual_start;
extern uintptr_t __data_virtual_end;

// SVC stack location
extern uintptr_t __svc_stack_limit;

// Kernel vmap
vmap_t kernel_vmap;

void _vmap_kernel_init() {
  // Get a reference to the kernel's pmap
  kernel_vmap.pmap = pmap_kernel();
  pmap_reference(kernel_vmap.pmap);

  // We need to create the regions: text, data, stack, and heap. And vm objects.
  kernel_vmap.regions = (vregion_t*)pmap_steal_memory(sizeof(vregion_t) * 4);

  // We need to get the start and end addresses of the virtual memory regions of the kernel
  vaddr_t vas[8] = {(uintptr_t)(&__text_virtual_start), (uintptr_t)(&__text_virtual_end), (uintptr_t)(&__data_virtual_start), (uintptr_t)(&__data_virtual_end), 0, 0, (uintptr_t)(&__svc_stack_limit), 0};
  
  // The kernel heap doesn't exist yet (hence heap_end = heap_start)
  vas[5] = vas[4];

  // The address of the start of the stack is in vas[7] (This address represents the end of the virtual memory region however). 
  // Since the stack grows downward, the end of stack (or start of the virtual memory region) is vas[7]-0x1000 (since we have defined the stacks
  // to be 4096 bytes in size)
  // TODO: PAGESIZE shouldn't be hardcoded. What if we change the size of the kernel stacks? Maybe have a STACKSIZE?
  vas[7] = vas[6]+PAGESIZE;

  kernel_vmap.text_start = vas[0];
  kernel_vmap.text_end = vas[1];
  kernel_vmap.data_start = vas[2];
  kernel_vmap.data_end = vas[3];
  kernel_vmap.stack_start = vas[7];
  
  // The kernel heap doesn't exist yet
  // These values need to be updated after the final pmap_steal_memory call
  kernel_vmap.heap_start = vas[4];
  kernel_vmap.heap_end = kernel_vmap.heap_end;

  // Now lets populate each of the vregion structs
  // Note the indexing operator only works here because we have allocated the vregions contiguously. Usually a linked list.
  vm_prot_t prots[4] = {VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_DEFAULT, VM_PROT_DEFAULT, VM_PROT_DEFAULT};
  for(uint32_t i = 0, num_regions = 4; i < num_regions; i++) {
    kernel_vmap.regions[i].vstart = vas[(i*2)];
    kernel_vmap.regions[i].vend = ROUND_PAGE(vas[(i*2)+1]);
    kernel_vmap.regions[i].vm_prot = prots[i];
    kernel_vmap.regions[i].needs_copy = kernel_vmap.regions[i].copy_on_write = 0;
    
    // Populate the amaps
    uint32_t num_pages = (uint32_t)((double)(kernel_vmap.regions[i].vend - kernel_vmap.regions[i].vstart) / (double)PAGESIZE);
    kernel_vmap.regions[i].aref.amap = (vm_amap_t*)pmap_steal_memory(sizeof(vm_amap_t));
    kernel_vmap.regions[i].aref.slotoff = 0;
    kernel_vmap.regions[i].aref.amap->maxslots = kernel_vmap.regions[i].aref.amap->nslots = num_pages;
    kernel_vmap.regions[i].aref.amap->refcount = 1;
    if(num_pages > 0) kernel_vmap.regions[i].aref.amap->aslots = (vm_anon_t**)pmap_steal_memory(sizeof(vm_anon_t*) * num_pages);

    // Populate the anon structs and put them in amap.aslots
    for(uint32_t j = 0; j < num_pages; j++) {
      vm_anon_t *anon = (vm_anon_t*)pmap_steal_memory(sizeof(vm_anon_t));
      anon->page = (vpage_t*)pmap_steal_memory(sizeof(vpage_t));
      anon->page->vaddr = kernel_vmap.regions[i].vstart + (j * PAGESIZE);
      kernel_vmap.regions[i].aref.amap->aslots[j] = anon;
      anon->refcount = 1;
    }
    
    if((i+1) != num_regions) kernel_vmap.regions[i].next = &kernel_vmap.regions[i+1];
    else kernel_vmap.regions[i].next = NULL;
  }

  // Now lets set up the (empty) heap region
  // So what is happening here? Initially the kernel heap is empty, meaning there are no pages in the heap region.
  // Okay, thats great. So when kheap_init is called, it wants to extend the heap region to a certain size and the function vmm_km_heap_extend should do that.
  // Alright. But whats the problem? Well for each virtual page that kheap_init wants to extend the heap region by, there needs to be a vm_anon_t and vpage_t struct 
  // associated with those pages. Not only that but a vm_amap_t structure needs to be allocated for the heap region.
  // The problem is now, where the hell do we put these structures? We can't put them on the heap because technically the heap hasn't even been fully initialized.
  // Now assume that the kheap has been initialized somehow. When kheap tries to allocate memory but sees that there isn't enough, it asks vmm_km_heap_extend to map 
  // more memory for the heap region. Again, vmm needs to allocate vm_anon_t and vpage_t structures and reallocate the vm_amap_t structure and again we can't use the
  // kheap because the kheap is asking vmm for more memory! The problem is cyclic dependency. kheap needs vmm_km_heap_extend to extend the heap region but vmm_km_heap_extend
  // needs kheap to allocate data structures which manage the new memory mappings!
  // So, the idea is to calculate the maximum potential size of the kheap and preallocate all the needed data structures. That way whenever vmm_km_heap_extend is called
  // it doesn't have to allocate any memory, it can just use what's already been allocated. Of course this means that kernel heap region structure can't be used with 
  // any other vmm function since it needs 'special' treatment.
  pmap_virtual_space(NULL, &kernel_vmap.heap_start);
  kernel_vmap.regions[2].vstart = kernel_vmap.regions[2].vend = kernel_vmap.heap_end = kernel_vmap.heap_start = ROUND_PAGE(kernel_vmap.heap_start);
  size_t max_heap_size = 0xFFFF000 - kernel_vmap.heap_start;
  uint32_t num_pages_needed = (uint32_t)((double)max_heap_size/(double)PAGESIZE);
  uint32_t num_bytes_needed = ROUND_PAGE(sizeof(uintptr_t)*num_pages_needed + sizeof(vm_anon_t)*num_pages_needed + sizeof(vpage_t)*num_pages_needed);
  kernel_vmap.regions[2].vstart = kernel_vmap.regions[2].vend = kernel_vmap.heap_end = kernel_vmap.heap_start += num_bytes_needed;

  kernel_vmap.regions[2].aref.slotoff = 0;
  kernel_vmap.regions[2].aref.amap->maxslots = kernel_vmap.regions[2].aref.amap->nslots = 0;
  kernel_vmap.regions[2].aref.amap->refcount = 1;
  kernel_vmap.regions[2].aref.amap->aslots = (vm_anon_t**)pmap_steal_memory(sizeof(vm_anon_t*) * num_pages_needed);

  for(uint32_t i = 0; i < num_pages_needed; i++) {
    vm_anon_t *anon = (vm_anon_t*)pmap_steal_memory(sizeof(vm_anon_t));
    anon->page = (vpage_t*)pmap_steal_memory(sizeof(vpage_t));
    anon->page->vaddr = UINT32_MAX;
    kernel_vmap.regions[2].aref.amap->aslots[i] = anon;
    anon->refcount = 1;
  }


}

void vmm_init() {
  // Initialize the kernel vmap
  _vmap_kernel_init();
}

vregion_t* vmm_region_lookup(vmap_t *vmap, vaddr_t va) {
  kassert(vmap != NULL);

  vregion_t *region;
  for(region = vmap->regions; region != NULL; region = region->next) {
    // Regions are sorted by increasing start addresses. If the va is less than the starting address of a region we
    // can reasonably assume that the va doesn't exist in any of the succeeding regions
    if(va < region->vstart) {
      region = NULL;
      break;
    }

    // We found the region that encompasses this va (The second comparison is for when the region start==end)
    if((va >= region->vstart && va < region->vend) || va == region->vstart) break;
  }

  return region;
}

// TODO: Wholly incomplete. Need allocate (or realloc) memory for amap and create new anons and vpages
size_t vmm_km_heap_extend(vregion_t *region, size_t size) {
  kassert(IS_KERNEL_REGION(region));
  kassert((UINT32_MAX - region->vend) > ROUND_PAGE(size));

  vaddr_t prev_vend = region->vend;
  region->vend += ROUND_PAGE(size);

  for(vaddr_t va = prev_vend; va < region->vend; va += PAGESIZE) {
    // Allocate a free page if one should be available else panic
    paddr_t pa = pmm_alloc();
    kassert(pa != UINTPTR_MAX);

    // TODO: Use pmap_enter here instead
    pmap_kenter_pa(va, pa, region->vm_prot, PMAP_WIRED | PMAP_WRITE_COMBINE); 

    // Enter the information into the amap
    region->aref.amap->aslots[(uint32_t)((double)(va-region->vstart)/(double)PAGESIZE)]->page->vaddr = va;
  }

  memset((vaddr_t*)prev_vend, 0, PAGESIZE);
  vmap_kernel()->heap_end = region->vend;
  
  uint32_t new_size = region->vend - region->vstart;
  region->aref.amap->maxslots = region->aref.amap->nslots = (uint32_t)((double)new_size/(double)PAGESIZE);

  return new_size;
}

