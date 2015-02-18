#ifndef __IRQ_H__
#define __IRQ_H__

#include <sys/types.h>

#if defined(REALVIEW_PB)
  #include <platform/realview-pb/irqtypes.h>
#elif defined(VERSATILE_PB)
  #include <platform/versatile-pb/irqtypes.h>
#else
  #error "Undefined platform"
#endif

typedef void (*isr_t)(void);

// Initialize IRQ systems, VIC, SIC...
void irq_init();

// This function enables IRQ for the specified source on the VIC and SIC
void irq_enable(irq_type_t);

// This function disables IRQ for the specified source on the VIC and SIC
void irq_disable(irq_type_t);

// This function should be called whenever an IRQ exception occurs
// It will return the irq type that caused the exception
irq_type_t irq_get();

// Returns the ISR function associated with the IRQ type
isr_t irq_get_isr(irq_type_t);

// Register and ISR for the specified IRQ source
void irq_register_isr(irq_type_t, isr_t);

#endif // __IRQ_H__

