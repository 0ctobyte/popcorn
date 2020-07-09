#ifndef _ARCH_EXCEPTION_H_
#define _ARCH_EXCEPTION_H_

#include <sys/types.h>

typedef enum {
    EXCEPTION_SYNC_SP_EL0     = 0,  // Synchronous exception from current exception level with SP_EL0
    EXCEPTION_IRQ_SP_EL0      = 1,  // IRQ exception from current exception level with SP_EL0
    EXCEPTION_FIQ_SP_EL0      = 2,  // FIQ exception from current exception level with SP_EL0
    EXCEPTION_SERR_SP_EL0     = 3,  // SError exception from current exception level with SP_EL0
    EXCEPTION_SYNC_SP_ELX     = 4,  // Synchronous exception from current exception level with SP_ELx where x > 0
    EXCEPTION_IRQ_SP_ELX      = 5,  // IRQ exception from current exception level with SP_ELx where x > 0
    EXCEPTION_FIQ_SP_ELX      = 6,  // FIQ exception from current exception level with SP_ELx where x > 0
    EXCEPTION_SERR_SP_ELX     = 7,  // SError exception from current exception level with SP_ELx where x > 0
    EXCEPTION_SYNC_LL_AARCH64 = 8,  // Synchronous exception from lower exception level using aarch64
    EXCEPTION_IRQ_LL_AARCH64  = 9,  // IRQ exception from lower exception level using aarch64
    EXCEPTION_FIQ_LL_AARCH64  = 10, // FIQ exception from lower exception level using aarch64
    EXCEPTION_SERR_LL_AARCH64 = 11, // SError exception from lower exception level using aarch64
    EXCEPTION_SYNC_LL_AARCH32 = 12, // Synchronous exception from lower exception level using aarch32
    EXCEPTION_IRQ_LL_AARCH32  = 13, // IRQ exception from lower exception level using aarch32
    EXCEPTION_FIQ_LL_AARCH32  = 14, // FIQ exception from lower exception level using aarch32
    EXCEPTION_SERR_LL_AARCH32 = 15, // SError exception from lower exception level using aarch632
} arch_exception_type_t;

typedef struct {
    uint64_t x[30];
    uint64_t lr;
    uint64_t sp;
    uint64_t elr;
    uint64_t spsr;
    uint64_t far;
    uint64_t esr;
} arch_context_t;

// Sets the vector base address register
void arch_exceptions_init(void);

// Given a pointer to an exception_state_t struct, it will pretty print the contents
void arch_exceptions_dump_state(arch_context_t *exc_context);

#endif // _ARCH_EXCEPTION_H_
