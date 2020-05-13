#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#define INTERRUPT_LOCK bool __en = interrupts_enabled(); interrupts_disable();
#define INTERRUPT_UNLOCK if(__en) interrupts_enable();

// Enables interrupts on the processor
void interrupts_enable(void);

// Disables interrupts on the processor
void interrupts_disable(void);

// Checks if interrupts are enabled on the processor
bool interrupts_enabled(void);

#endif // __INTERRUPTS_H__

