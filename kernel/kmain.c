#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/evt.h>
#include <kernel/mm.h>
#include <kernel/pmap.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/interrupts.h>
#include <kernel/kheap.h>

#include <lib/asm.h>

#include <platform/irq.h>
#include <platform/iomem.h>

extern uintptr_t exception_vector_table;

void _print_bins() {
  for(uint32_t i = 0; i < (sizeof(bins)>>2); i++) {
    kprintf("%u byte bin\n", 4 << i);
    vaddr_t head = bins[i];

    for(vaddr_t j = head, k = 0; j != 0; j = *(vaddr_t*)j, k++) {
      kprintf("block %u @ %#x\n", k, j);
    }
    kprintf("\n");
  }
}

void kmain(void) {
  // Setup the exception vector table
	evt_init();

  // Initializes the pmap and pmm systems
  // We need to also map device memory here (like the GIC and UART)
  // We need to map the GIC in order to set up the IRQ system
  // We need to map UART in order to use kprintf!
  pmap_init();
 
  // TODO: This shouldn't be here! Only temporary
  // Map the UART, VIC/SIC/GIC
  pmap_kenter_pa(R_UART0_VBASE, R_UART0_PBASE, VM_PROT_DEFAULT, PMAP_NOCACHE);
  if(IS_WITHIN_BOUNDS(R_UART0_PBASE)) pmm_reserve(R_UART0_PBASE);

	kprintf("Kernel: Initializing...\n");
  kprintf("Kernel: Mem base addr = %#x, Mem size = %u MB (%#x), Page size = %u bytes (%#x)\n", MEMBASEADDR, (MEMSIZE >> 20), MEMSIZE, PAGESIZE, PAGESIZE);
  kprintf("Kernel: Exception vector table set up at: %#p\n", &exception_vector_table);
  kprintf("kernel: pmap/pmm init. Page directory located at physical address: %#x\n", pmap_kernel()->pgd_pa);

  kprintf("Kernel: vmm init\n");
  vmm_init();
  kheap_init();

  kprintf("\nKERNEL VIRTUAL MEMORY MAP\n");
  for(vregion_t *kregions = vmap_kernel()->regions; kregions != NULL; kregions = kregions->next) {
    kprintf("vstart = %#x, vend = %#x\n", kregions->vstart, kregions->vend);

    kprintf("Access Protections: ");
    if(kregions->vm_prot & VM_PROT_READ) kprintf("r");
    if(kregions->vm_prot & VM_PROT_WRITE) kprintf("w");
    if(kregions->vm_prot & VM_PROT_EXECUTE) kprintf("x");
    kprintf("\n");

    if(kregions->aref.amap != NULL) {
      uint32_t nslots = (uint32_t)((double)(kregions->vend - kregions->vstart) / (double)PAGESIZE);
      for(uint32_t i = 0; i < nslots; i++) {
       vm_anon_t *anon = kregions->aref.amap->aslots[kregions->aref.slotoff + i];
       kprintf("Page %u: %#x\n", i, anon->page->vaddr);
      }
    }
    kprintf("\n");
  }

  vaddr_t *adr = (vaddr_t*)kheap_alloc(1<<21);
  _print_bins();
  vaddr_t *adr1 = (vaddr_t*)kheap_alloc(1<<21);
  _print_bins();
  vaddr_t *adr2 = (vaddr_t*)kheap_alloc(PAGESIZE<<1);
  _print_bins();

  kprintf("Allocated\n");

  kheap_free(adr);
  _print_bins();
  kheap_free(adr1);
  _print_bins();
  kheap_free(adr2);
  _print_bins();

  // Enable interrupts on the CPU
  kprintf("Kernel: Enabling interrupts on the CPU\n");
  interrupts_enable();

  // Setup the IRQ system
	//irq_init();
}

