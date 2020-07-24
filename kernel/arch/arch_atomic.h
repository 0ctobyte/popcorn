/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _ARCH_ATOMIC_H_
#define _ARCH_ATOMIC_H_

typedef long atomic_t;

// Atomically tests if value at ptr is 0 and if so sets it to val
// Returns 0 on success, otherwise failure to set
#define arch_atomic_test_and_set(ptr, val)\
({\
    int result;\
    asm ("ldxr %0, [%1]\n"\
         "cbnz %0, _exit%=\n"\
         "stxr %w0, %2, [%1]\n"\
         "_exit%=:\n"\
         : "=r" (result)\
         : "r" (ptr), "r" (val));\
    result;\
})

// Atomically tests specified bit in the value at ptr is 0 and if so sets it to 1
// Returns 0 on success, otherwise failure to set
#define arch_atomic_test_and_set_bit(ptr, bit)\
({\
    int result;\
    asm ("ldxr %0, [%1]\n"\
         "tst %0, %2\n"\
         "bne _exit%=\n"\
         "orr x3, %0, %2\n"\
         "stxr %w0, x3, [%1]\n"\
         "_exit%=:\n"\
         : "+r" (result)\
         : "r" (ptr), "r" (bit)\
         : "x3");\
    result;\
})

// Atomically increment the value
#define arch_atomic_inc(v)\
({\
    atomic_t result;\
    asm ("loop%=:\n"\
         "ldxr x1, [%1]\n"\
         "add x2, x1, #1\n"\
         "stxr w3, x2, [%1]\n"\
         "cbnz w3, loop%=\n"\
         "mov %0, x1\n"\
         : "=r" (result)\
         : "r" (v)\
         : "x1", "x2", "w3");\
    result;\
})

// Atomically decrement the value
#define arch_atomic_dec(v)\
({\
    atomic_t result;\
    asm ("loop%=:\n"\
         "ldxr x1, [%1]\n"\
         "sub x2, x1, #1\n"\
         "stxr w3, x2, [%1]\n"\
         "cbnz w3, loop%=\n"\
         "mov %0, x1\n"\
         : "=r" (result)\
         : "r" (v)\
         : "x1", "x2", "w3");\
    result;\
})

#endif // _ARCH_ATOMIC_H_
