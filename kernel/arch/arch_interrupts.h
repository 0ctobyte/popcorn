#ifndef _ARCH_INTERRUPTS_H_
#define _ARCH_INTERRUPTS_H_

#include <sys/types.h>

#define INTERRUPT_LOCK bool __en = interrupts_is_enabled(); interrupts_disable();
#define INTERRUPT_UNLOCK if(__en) interrupts_enable();

// Enables interrupts on the processor
#define arch_interrupts_enable() asm inline ("msr DAIFclr, #0x3")

// Disables interrupts on the processor
#define arch_interrupts_disable() asm inline ("msr DAIFset, #0x3")

// Checks if interrupts are enabled on the processor
#define arch_interrupts_is_enabled()\
({\
    bool result;\
    asm inline ("mrs %0, DAIF\n"\
                "tst %0, #0xc\n"\
                "cset %0, eq\n"\
                : "+r" (result) ::);\
    result;\
})

#endif // _ARCH_INTERRUPTS_H_
