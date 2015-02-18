#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/panic.h>
#include <kernel/evt.h>
#include <kernel/mm.h>
#include <kernel/pmap.h>
#include <kernel/pmm.h>
#include <kernel/interrupts.h>
#include <lib/asm.h>

#include <platform/irq.h>
#include <platform/iomem.h>

void data_abort_handler(void *p) {
  uint32_t *rdump = (uint32_t*)p;

  kprintf("dump addr: %#x\n", (uintptr_t)rdump);

  for(uint32_t i = 0; i < 16; i++) {
    kprintf("R%u: %#x\n", i, rdump[i]);
  }

  panic("DATA ABORT\n");
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

	kprintf("Initializing kernel...\n");
	kprintf("what\n");

  // Enable interrupts on the CPU
  interrupts_enable();

  evt_register_handler(EVT_DATA_ABORT, data_abort_handler);

  kprintf("%u\n", REG_RD32(R_GIC0_VBASE));

  // Setup the IRQ system
	//irq_init();
}

