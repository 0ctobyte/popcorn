#ifndef _ARCH_INTERRUPTS_H_
#define _ARCH_INTERRUPTS_H_

#include <sys/types.h>

#define INTERRUPT_LOCK bool __en = interrupts_enabled(); interrupts_disable();
#define INTERRUPT_UNLOCK if(__en) interrupts_enable();

// Enables interrupts on the processor
void arch_interrupts_enable(void);

// Disables interrupts on the processor
void arch_interrupts_disable(void);

// Checks if interrupts are enabled on the processor
bool arch_interrupts_enabled(void);

#endif // _ARCH_INTERRUPTS_H_
