#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#define INTERRUPT_LOCK bool __en = interrupts_enabled(); interrupts_disable();
#define INTERRUPT_UNLOCK if(__en) interrupts_enable();

// Enables interrupts on the processor
void interrupts_enable();

// Disables interrupts on the processor
void interrupts_disable();

// Checks if interrupts are enabled on the processor
bool interrupts_enabled();

#endif // __INTERRUPTS_H__

