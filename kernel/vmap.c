#include <kernel/vmap.h>
#include <kernel/kassert.h>
#include <kernel/pmm.h>
#include <kernel/kheap.h>

#include <string.h>

// Linker symbols
extern uintptr_t __kernel_virtual_start;
extern uintptr_t __kernel_virtual_end;

// SVC stack location
extern uintptr_t __el1_stack_limit;

// Kernel vmap
vmap_t kernel_vmap;

// The end of unmanaged kernel virtual memory. This is used by vmm_km_zalloc to allocate unmanaged memory before kheap is initialized
static vaddr_t kernel_vend;

// TODO: Refactor this disgusting code
//void _vmap_kernel_init() {
//    // Get a reference to the kernel's pmap
//    kernel_vmap.pmap = pmap_kernel();
//    pmap_reference(kernel_vmap.pmap);
//
//    // We need to create the regions: text/data, stack, and heap. And vm objects.
//    kernel_vmap.regions = (vregion_t*)vmm_km_zalloc(sizeof(vregion_t) * 3);
//
//    // We need to get the start and end addresses of the virtual memory regions of the kernel
//    vaddr_t vas[6] = {(uintptr_t)(&__kernel_virtual_start), (uintptr_t)(&__kernel_virtual_end), 0, 0, (uintptr_t)(&__el1_stack_limit), 0};
//
//    // The address of the start of the stack is in vas[5] (This address represents the end of the virtual memory region however).
//    // Since the stack grows downward, the end of stack (or start of the virtual memory region) is vas[5]-0x1000 (since we have defined the stacks
//    // to be 4096 bytes in size)
//    // TODO: PAGESIZE shouldn't be hardcoded. What if we change the size of the kernel stacks? Maybe have a STACKSIZE?
//    vas[5] = vas[4]+PAGESIZE;
//
//    kernel_vmap.text_start = vas[0];
//    kernel_vmap.text_end = vas[1];
//    kernel_vmap.data_start = vas[0];
//    kernel_vmap.data_end = vas[1];
//    kernel_vmap.stack_start = vas[5];
//
//    // The kernel heap doesn't exist yet
//    // These values need to be updated after the final vmm_km_zalloc call
//    kernel_vmap.heap_start = vas[2];
//    kernel_vmap.heap_end = kernel_vmap.heap_end;
//
//    // Now lets populate each of the vregion structs
//    // Note the indexing operator only works here because we have allocated the vregions contiguously. Usually a linked list.
//    vmm_prot_t prots[3] = {VM_PROT_ALL, VM_PROT_DEFAULT, VM_PROT_DEFAULT};
//    for(unsigned long i = 0, num_regions = 3; i < num_regions; i++) {
//        kernel_vmap.regions[i].vstart = vas[(i*2)];
//        kernel_vmap.regions[i].vend = ROUND_PAGE(vas[(i*2)+1]);
//        kernel_vmap.regions[i].vm_prot = prots[i];
//        kernel_vmap.regions[i].needs_copy = kernel_vmap.regions[i].copy_on_write = 0;
//
//        // Populate the amaps
//        unsigned long num_pages = (unsigned long)((double)(kernel_vmap.regions[i].vend - kernel_vmap.regions[i].vstart) / (double)PAGESIZE);
//        if(num_pages > 0) {
//            kernel_vmap.regions[i].aref.amap = (vm_amap_t*)vmm_km_zalloc(sizeof(vm_amap_t));
//            kernel_vmap.regions[i].aref.slotoff = 0;
//            kernel_vmap.regions[i].aref.amap->maxslots = kernel_vmap.regions[i].aref.amap->nslots = num_pages;
//            kernel_vmap.regions[i].aref.amap->refcount = 1;
//            kernel_vmap.regions[i].aref.amap->aslots = (vm_anon_t**)vmm_km_zalloc(sizeof(vm_anon_t*) * num_pages);
//
//            // Populate the anon structs and put them in amap.aslots
//            for(unsigned long j = 0; j < num_pages; j++) {
//                vm_anon_t *anon = (vm_anon_t*)vmm_km_zalloc(sizeof(vm_anon_t));
//                anon->page = (vmm_page_t*)vmm_km_zalloc(sizeof(vmm_page_t));
//                anon->page->vaddr = kernel_vmap.regions[i].vstart + (j * PAGESIZE);
//                kernel_vmap.regions[i].aref.amap->aslots[j] = anon;
//                anon->refcount = 1;
//            }
//        }
//
//        if((i+1) != num_regions) kernel_vmap.regions[i].next = &kernel_vmap.regions[i+1];
//        else kernel_vmap.regions[i].next = NULL;
//    }
//}

void vmm_init(void) {
    // Initialize the kernel vmap
    //_vmap_kernel_init();
}

vaddr_t vmm_km_zalloc(size_t size) {
    //// Pre kernel heap unmanaged memory allocator
    //// This should not only be used before kheap_init has been called
    //static vaddr_t placement_addr = 0;
    //if(placement_addr == 0) {
    //    pmap_virtual_space(NULL, &kernel_vend);
    //    placement_addr = kernel_vend;
    //}

    //// Make sure enough memory is left!
    //kassert((UINT32_MAX-placement_addr) >= size);

    //vaddr_t start = placement_addr;
    //vaddr_t end = placement_addr + size;

    //// Allocate a new page if there is not enough memory
    //if(end >= kernel_vend) {
    //    // Loop through and allocate pages until we have enough memory to serve the requested size
    //    for( ; kernel_vend < end; kernel_vend+=PAGESIZE) {
    //        paddr_t pa = pmm_alloc();
    //        pmap_kenter_pa(kernel_vend, pa, VM_PROT_DEFAULT, PMAP_WRITE_BACK);
    //    }
    //}

    //// Zero the memory
    //memset((void*)placement_addr, 0x0, size);

    //placement_addr = end;

    return 0;
}

// TODO: This function shouldn't need to exist. Find another way
void vmm_km_heap_init(void) {
    //// Now lets set up the (empty) heap region
    //// So what is happening here? Initially the kernel heap is empty, meaning there are no pages in the heap region.
    //// Okay, thats great. So when kheap_init is called, it wants to extend the heap region to a certain size and the function vmm_km_heap_extend should do that.
    //// Alright. But whats the problem? Well for each virtual page that kheap_init wants to extend the heap region by, there needs to be a vm_anon_t and vmm_page_t struct
    //// associated with those pages. Not only that but a vm_amap_t structure needs to be allocated for the heap region.
    //// The problem is now, where the hell do we put these structures? We can't put them on the heap because technically the heap hasn't even been fully initialized.
    //// Now assume that the kheap has been initialized somehow. When kheap tries to allocate memory but sees that there isn't enough, it asks vmm_km_heap_extend to map
    //// more memory for the heap region. Again, vmm needs to allocate vm_anon_t and vmm_page_t structures and reallocate the vm_amap_t structure and again we can't use the
    //// kheap because the kheap is asking vmm for more memory! The problem is cyclic dependency. kheap needs vmm_km_heap_extend to extend the heap region but vmm_km_heap_extend
    //// needs kheap to allocate data structures which manage the new memory mappings!
    //// So, the idea is to calculate the maximum potential size of the kheap and preallocate all the needed data structures. That way whenever vmm_km_heap_extend is called
    //// it doesn't have to allocate any memory, it can just use what's already been allocated. Of course this means that kernel heap region structure can't be used with
    //// any other vmm function since it needs 'special' treatment.
    //kernel_vmap.heap_start = kernel_vend;
    //kernel_vmap.heap_start = ROUND_PAGE(kernel_vmap.heap_start);

    //// Theoretical max size of the heap, the heap should never grow this large. As a matter of fact, we are overestimating since this doesn't take into account the space
    //// needed to store the vmm structures that will eat some of the heap space.
    //// TODO: 0xFFFF0000? What value should this be? UINT32_MAX?
    //size_t max_heap_size = ROUND_PAGE(0xFFFF0000 - kernel_vmap.heap_start);
    //unsigned long num_pages_needed = (unsigned long)((double)max_heap_size/(double)PAGESIZE);

    //// Set up the vmm data structures needed to store the all the information about the heap
    //kernel_vmap.regions[2].aref.amap = (vm_amap_t*)vmm_km_zalloc(sizeof(vm_amap_t));
    //kernel_vmap.regions[2].aref.slotoff = 0;
    //kernel_vmap.regions[2].aref.amap->maxslots = kernel_vmap.regions[2].aref.amap->nslots = 0;
    //kernel_vmap.regions[2].aref.amap->refcount = 1;
    //kernel_vmap.regions[2].aref.amap->aslots = (vm_anon_t**)vmm_km_zalloc(sizeof(vm_anon_t*) * num_pages_needed);

    //for(unsigned long i = 0; i < num_pages_needed; i++) {
    //    vm_anon_t *anon = (vm_anon_t*)vmm_km_zalloc(sizeof(vm_anon_t));
    //    anon->page = (vmm_page_t*)vmm_km_zalloc(sizeof(vmm_page_t));
    //    anon->page->vaddr = UINT32_MAX;
    //    kernel_vmap.regions[2].aref.amap->aslots[i] = anon;
    //    anon->refcount = 1;
    //}

    //// Update the start of the heap
    //kernel_vmap.heap_start = kernel_vend;
    //kernel_vmap.regions[2].vstart = kernel_vmap.regions[2].vend = kernel_vmap.heap_end = kernel_vmap.heap_start = ROUND_PAGE(kernel_vmap.heap_start);
}

// TODO: This function shouldn't need to exist. Find another way
vaddr_t vmm_km_heap_extend(size_t size) {
    //vregion_t *region = &vmap_kernel()->regions[2];
    //kassert((UINT32_MAX - region->vend) > ROUND_PAGE(size));


    //vaddr_t prev_vend = region->vend;
    //region->vend += ROUND_PAGE(size);

    //for(vaddr_t va = prev_vend; va < region->vend; va += PAGESIZE) {
    //    // Allocate a free page if one should be available else panic
    //    paddr_t pa = pmm_alloc();
    //    kassert(pa != UINTPTR_MAX);

    //    // TODO: Use pmap_enter here instead
    //    pmap_kenter_pa(va, pa, region->vm_prot, PMAP_WIRED | PMAP_WRITE_COMBINE);

    //    // Enter the information into the amap
    //    region->aref.amap->aslots[(unsigned long)((double)(va-region->vstart)/(double)PAGESIZE)]->page->vaddr = va;
    //}

    //memset((vaddr_t*)prev_vend, 0, PAGESIZE);
    //vmap_kernel()->heap_end = region->vend;

    //unsigned long new_size = region->vend - region->vstart;
    //region->aref.amap->maxslots = region->aref.amap->nslots = (unsigned long)((double)new_size/(double)PAGESIZE);

    return 0;
}
