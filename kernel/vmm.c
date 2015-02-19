#include <kernel/vmm.h>

// Kernel vmap
vmap_t kernel_vmap;

void _vmap_kernel_init() {
  // Get a reference to the kernel's pmap
  kernel_vmap.pmap = pmap_kernel();
  pmap_reference(kernel_vmap.pmap);

  // We need to create the regions: text, data, stack, and heap. And vm objects.
  kernel_vmap.regions = (vregion_t*)pmap_steal_memory(sizeof(vregion_t) * 4);
  vobject_t *objects = (vobject_t*)pmap_steal_memory(sizeof(vobject_t) * 4);

  // We need to get the start and end addresses of the virtual memory regions of the kernel
  vaddr_t vas[8];
  pmap_virtual_space(&vas[0], &vas[1], &vas[2], &vas[3], &vas[4], &vas[7]);
  
  // The kernel heap doesn't exist yet (hence heap_end = heap_start)
  vas[5] = vas[4];

  // The address of the start of the stack is in vas[7] (This address represents the end of the virtual memory region however). 
  // Since the stack grows downward, the end of stack (or start of the virtual memory region) is vas[7]-0x1000 (since we have defined the stacks
  // to be 4096 bytes in size)
  // TODO: 0x1000 shouldn't be hardcoded. What if we change the size of the kernel stacks?
  vas[6] = vas[7]-0x1000;

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
    kernel_vmap.regions[i].start = vas[i];
    kernel_vmap.regions[i].end = vas[i+1];
    kernel_vmap.regions[i].prot = prots[i];
    kernel_vmap.regions[i].obj = objects[i];
    if((i+1) != num_regions) kernel_vmap.regions[i].next = &kernel_vmap.regions[i+1];
    else kernel_vmap.regions[i].next = NULL;
  }

  // Now we need to populate each of the objects. All kernel objects are anonymous (zero-fill).
}

void vmm_init() {
  // Initialize the kernel vmap
  _vmap_kernel_init();
}


