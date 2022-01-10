/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _ARCH_INTERRUPTS_H_
#define _ARCH_INTERRUPTS_H_

#include <sys/types.h>

#define INTERRUPT_LOCK bool __en = interrupts_is_enabled(); interrupts_disable();
#define INTERRUPT_UNLOCK if(__en) interrupts_enable();

// Enables interrupts on the processor
#define arch_interrupts_enable() asm volatile ("msr DAIFclr, #0x3")

// Disables interrupts on the processor
#define arch_interrupts_disable() asm volatile ("msr DAIFset, #0x3")

// Checks if interrupts are enabled on the processor
#define arch_interrupts_is_enabled()\
({\
    unsigned long result;\
    asm ("mrs %0, DAIF\n"\
         "tst %0, #0xc0\n"\
         "csetm %0, eq\n"\
         : "+r" (result) :);\
    (bool)result;\
})

#endif // _ARCH_INTERRUPTS_H_
