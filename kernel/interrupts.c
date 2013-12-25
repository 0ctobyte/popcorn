// All interrupt vectors go here, except for the IRQ ISR

#include <stdint.h>

#include "../include/kernel/interrupts.h"

void __attribute__((interrupt)) undef_handler(void) { for(;;); }
void __attribute__((interrupt)) swi_handler(void) { for(;;); }
void __attribute__((interrupt)) prefetch_abort_handler(void) { for(;;); }
void __attribute__((interrupt)) data_abort_handler(void) { for(;;); }
void __attribute__((interrupt)) fiq_handler(void) { for(;;); }
void __attribute__((interrupt)) irq_handler(void) { for(;;); }
