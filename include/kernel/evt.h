#ifndef __EVT_H__
#define __EVT_H__

typedef enum {
    EVT_SYNC_SP_EL0     = 0,  // Synchronous exception from current exception level with SP_EL0
    EVT_IRQ_SP_EL0      = 1,  // IRQ exception from current exception level with SP_EL0
    EVT_FIQ_SP_EL0      = 2,  // FIQ exception from current exception level with SP_EL0
    EVT_SERR_SP_EL0     = 3,  // SError exception from current exception level with SP_EL0
    EVT_SYNC_SP_ELX     = 4,  // Synchronous exception from current exception level with SP_ELx where x > 0
    EVT_IRQ_SP_ELX      = 5,  // IRQ exception from current exception level with SP_ELx where x > 0
    EVT_FIQ_SP_ELX      = 6,  // FIQ exception from current exception level with SP_ELx where x > 0
    EVT_SERR_SP_ELX     = 7,  // SError exception from current exception level with SP_ELx where x > 0
    EVT_SYNC_LL_AARCH64 = 8,  // Synchronous exception from lower exception level using aarch64
    EVT_IRQ_LL_AARCH64  = 9,  // IRQ exception from lower exception level using aarch64
    EVT_FIQ_LL_AARCH64  = 10, // FIQ exception from lower exception level using aarch64
    EVT_SERR_LL_AARCH64 = 11, // SError exception from lower exception level using aarch64
    EVT_SYNC_LL_AARCH32 = 12, // Synchronous exception from lower exception level using aarch32
    EVT_IRQ_LL_AARCH32  = 13, // IRQ exception from lower exception level using aarch32
    EVT_FIQ_LL_AARCH32  = 14, // FIQ exception from lower exception level using aarch32
    EVT_SERR_LL_AARCH32 = 15, // SError exception from lower exception level using aarch632
} evt_type_t;

typedef void (*evt_handler_t)(void*);

// Sets the vector base address register
void evt_init(void);

// Register the function specified by evt_handler_t to the
// exception type specified by evt_type_t
void evt_register_handler(evt_type_t, evt_handler_t);

#endif // __EVT_H__

