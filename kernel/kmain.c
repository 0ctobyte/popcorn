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

void print_heap_stats() {
  size_t heap_free, heap_allocated, num_free_blocks, num_used_blocks, heap_size;
  kheap_stats(&heap_free, &heap_allocated, &num_free_blocks, &num_used_blocks);
  heap_size = heap_allocated + heap_free;
  kprintf("\nHEAP STATISTICS\n");
  kprintf("Heap Utilization: %llu free | %llu allocated / %llu total size = %f%%\n", heap_free, heap_allocated, heap_size, ((double)heap_allocated/(double)heap_size)*100.0);
  kprintf("Heap blocks: used = %llu | free = %llu\n", num_used_blocks, num_free_blocks);
}

void data_abort_handler(void *d) {
  uint32_t *registers = (uint32_t*)d;

  kprintf("\nDATA ABORT EXCEPTION\n");
  kprintf("Fault Address: %#x\n", registers[0]);
  kprintf("Fault Status: %#x\n", registers[1]);
  kprintf("Cache Maintenance Fault? %s\n", (registers[1] & (1 << 13)) ? "yes" : "no");
  kprintf("Fault on Write or Read? %s\n", (registers[1] & (1 << 11)) ? "write" : "read");
  uint32_t fault_status = ((registers[1] & (1 << 10)) >> 6) | (registers[1] & 0xF);

  if(fault_status == 0x1) kprintf("Alignment Fault");
  else if(fault_status == 0x4) kprintf("Instruction Cache Maintenance Fault");
  else if(fault_status == 0xC) kprintf("Synchronous External Abort on 1st Level Translation Table Walk");
  else if(fault_status == 0xE) kprintf("Synchronous External Abort on 2nd Level Translation Table Walk");
  else if(fault_status == 0x1C) kprintf("Synchronous Parity Error on 1st Level Translation Table Walk");
  else if(fault_status == 0x1E) kprintf("Synchronous Parity Error on 2nd Level Translation Table Walk");
  else if(fault_status == 0x5) kprintf("1st Level Translation Fault");
  else if(fault_status == 0x7) kprintf("2nd Level Translation Fault");
  else if(fault_status == 0x3) kprintf("1st Level Access Flag Fault");
  else if(fault_status == 0x6) kprintf("2nd Level Access Flag Fault");
  else if(fault_status == 0x9) kprintf("1st Level Domain Fault");
  else if(fault_status == 0xB) kprintf("2nd Level Domain Fault");
  else if(fault_status == 0xD) kprintf("1st Level Permission Fault");
  else if(fault_status == 0xF) kprintf("2nd Level Permission Fault");
  else if(fault_status == 0x2) kprintf("Debug Event");
  else if(fault_status == 0x8) kprintf("Synchronous External Abort");
  else if(fault_status == 0x10) kprintf("TLB Conflict Abort");
  else if(fault_status == 0x19) kprintf("Synchronous Parity Error on Memory Access");
  else if(fault_status == 0x16) kprintf("Asynchronous External Abort");
  else if(fault_status == 0x18) kprintf("Asynchronous Parity Error on Memory Access");
  else kprintf("Unknown Abort");

  kprintf("\n");
  kprintf("Register Dump\n");
  kprintf("SPSR: %#x\n", registers[2]);
  for(uint32_t i = 3; i < 16; ++i) kprintf("R%u: %#x\n", (i-3), registers[i]);
  kprintf("LR: %#x\n", registers[16]);

  panic("HALTING\n");
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

  evt_register_handler(EVT_DATA_ABORT, data_abort_handler);

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

  // Enable interrupts on the CPU
  kprintf("Kernel: Enabling interrupts on the CPU\n");
  interrupts_enable();

  uint32_t s = 5;
  void *ptr[5];

  for(uint32_t i = 0; i < s; i++) {
    ptr[i] = kheap_alloc((1 << 2) << (10-i));
    print_heap_stats();
  }

  for(uint32_t i = 0; i < s; i++) {
    kheap_free(ptr[i]);
    print_heap_stats();
  }

  void *p = kheap_alloc(0x4000);
  kprintf("%#p, %#x\n", p, (uintptr_t)p & (~0x3fff));

  // Setup the IRQ system
	//irq_init();
}

