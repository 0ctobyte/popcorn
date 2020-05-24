#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/exceptions.h>
#include <kernel/interrupts.h>
#include <kernel/pmap.h>
#include <kernel/vmm.h>
#include <kernel/kheap.h>
#include <kernel/devicetree.h>

#include <lib/asm.h>

unsigned long MEMBASEADDR;
unsigned long MEMSIZE;

void print_heap_stats() {
    size_t heap_free, heap_allocated, num_free_blocks, num_used_blocks, heap_size;
    kheap_stats(&heap_free, &heap_allocated, &num_free_blocks, &num_used_blocks);
    heap_size = heap_allocated + heap_free;
    kprintf("\nHEAP STATISTICS\n");
    kprintf("Heap Utilization: %llu free | %llu allocated / %llu total size = %f%%\n", heap_free, heap_allocated, heap_size, ((double)heap_allocated/(double)heap_size)*100.0);
    kprintf("Heap blocks: used = %llu | free = %llu\n", num_used_blocks, num_free_blocks);
}

#include <string.h>
void kmain(void) {
    // Setup the exception vector table
    exceptions_init();

    if (!devicetree_find_memory(&MEMBASEADDR, &MEMSIZE)) HALT();

    kprintf("Hello World\n");

    // Initializes the pmap and pmm systems
    // We need to also map device memory here (like the GIC and UART)
    // We need to map the GIC in order to set up the IRQ system
    // We need to map UART in order to use kprintf!
    pmap_init();
    kprintf("pmap_init() - done!\n");
    vaddr_t vstartp, vendp;
    pmap_virtual_space(&vstartp, &vendp);
    kprintf("virtual space: %p - %p\n", vstartp, vendp);

    //// TODO: This shouldn't be here! Only temporary
    //// Map the UART, VIC/SIC/GIC
    //pmap_kenter_pa(R_UART0_VBASE, R_UART0_PBASE, VM_PROT_DEFAULT, PMAP_NOCACHE);
    //if(IS_WITHIN_BOUNDS(R_UART0_PBASE)) pmm_reserve(R_UART0_PBASE);

    //evt_register_handler(EVT_SYNC_SP_ELX, data_abort_handler);

    //kprintf("Kernel: Initializing...\n");
    //kprintf("Kernel: Mem base addr = %#x, Mem size = %u MB (%#x), Page size = %u bytes (%#x)\n", MEMBASEADDR, (MEMSIZE >> 20), MEMSIZE, PAGESIZE, PAGESIZE);
    //kprintf("Kernel: Exception vector table set up at: %#p\n", &exception_vector_table);
    //kprintf("kernel: pmap/pmm init. Page directory located at physical address: %#x\n", pmap_kernel()->pgd_pa);

    //kprintf("Kernel: vmm init\n");
    //vmm_init();
    //kheap_init();

    //kprintf("\nKERNEL VIRTUAL MEMORY MAP\n");
    //for(vregion_t *kregions = vmap_kernel()->regions; kregions != NULL; kregions = kregions->next) {
    //    kprintf("vstart = %#x, vend = %#x\n", kregions->vstart, kregions->vend);

    //    kprintf("Access Protections: ");
    //    if(kregions->vm_prot & VM_PROT_READ) kprintf("r");
    //    if(kregions->vm_prot & VM_PROT_WRITE) kprintf("w");
    //    if(kregions->vm_prot & VM_PROT_EXECUTE) kprintf("x");
    //    kprintf("\n");

    //    if(kregions->aref.amap != NULL) {
    //        unsigned long nslots = (unsigned long)((double)(kregions->vend - kregions->vstart) / (double)PAGESIZE);
    //        for(unsigned long i = 0; i < nslots; i++) {
    //            vm_anon_t *anon = kregions->aref.amap->aslots[kregions->aref.slotoff + i];
    //            kprintf("Page %u: %#x\n", i, anon->page->vaddr);
    //        }
    //    }
    //    kprintf("\n");
    //}

    //// Enable interrupts on the CPU
    //kprintf("Kernel: Enabling interrupts on the CPU\n");
    //interrupts_enable();

    //unsigned long s = 5;
    //void *ptr[5];

    //for(unsigned long i = 0; i < s; i++) {
    //    ptr[i] = kheap_alloc((1 << 2) << (10-i));
    //    print_heap_stats();
    //}

    //for(unsigned long i = 0; i < s; i++) {
    //    kheap_free(ptr[i]);
    //    print_heap_stats();
    //}

    //void *p = kheap_alloc(0x4000);
    //kprintf("%#p, %#x\n", p, (uintptr_t)p & (~0x3fff));

    // Setup the IRQ system
    //irq_init();
}
