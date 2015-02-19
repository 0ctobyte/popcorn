#include <kernel/vmm.h>


// Kernel vmap
vmap_t kernel_vmap;

void _vmap_kernel_init() {
  // Get a reference to the kernel's pmap
  kernel_vmap.pmap = pmap_kernel();
  pmap_reference(kernel_vmap.pmap);

  // We need to create the regions: text, data, stack, and heap. And vm objects.
  kernel_vmap.regions = (vregion_t*)pmap_steal_memory(sizeof(vregion_t) * 4);
  vm_amap_t *amaps = (vm_amap_t*)pmap_steal_memory(sizeof(vm_amap_t) * 4);

  // We need to get the start and end addresses of the virtual memory regions of the kernel
  vaddr_t vas[8];
  pmap_virtual_space(&vas[0], &vas[1], &vas[2], &vas[3], &vas[4], &vas[7]);
  
  // The kernel heap doesn't exist yet (hence heap_end = heap_start)
  vas[5] = vas[4];

  // The address of the start of the stack is in vas[7] (This address represents the end of the virtual memory region however). 
  // Since the stack grows downward, the end of stack (or start of the virtual memory region) is vas[7]-0x1000 (since we have defined the stacks
  // to be 4096 bytes in size)
  // TODO: PAGESIZE shouldn't be hardcoded. What if we change the size of the kernel stacks? Maybe have a STACKSIZE?
  vas[6] = vas[7]-PAGESIZE;

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
    // TODO: Allocate memory for pages
    uint32_t num_pages = (uint32_t)((double)(kernel_vmap.regions[i].vend - kernel_vmap.regions[i].vstart) / (double)PAGESIZE);
    kernel_vmap.regions[i].aref.amap = &amaps[i];
    kernel_vmap.regions[i].aref.slotoff = 0;
    amaps[i].maxslots = amaps[i].nslots = num_pages;
    amaps[i].refcount = 1;
    if(num_pages > 0) amaps[i].aslots = (vm_anon_t**)pmap_steal_memory(sizeof(vm_anon_t*) * num_pages);

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

  // Now lets set up the heap
}

void vmm_init() {
  // Initialize the kernel vmap
  _vmap_kernel_init();
}


