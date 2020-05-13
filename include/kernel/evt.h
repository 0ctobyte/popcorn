#ifndef __EVT_H__
#define __EVT_H__

typedef enum {
    EVT_UNDEFINED       = 0, // Undefined instruction exception
    EVT_SWI             = 1, // Software interrupt exception
    EVT_PREFETCH_ABORT  = 2, // Prefetch abort exception
    EVT_DATA_ABORT      = 3, // Data abort exception
    EVT_IRQ             = 4, // IRQ exception
    EVT_FIQ             = 5, // FIQ exception
} evt_type_t;

typedef void (*evt_handler_t)(void*);

// Sets the vector base address register
void evt_init(void);

// Register the function specified by evt_handler_t to the
// exception type specified by evt_type_t
void evt_register_handler(evt_type_t, evt_handler_t);

#endif // __EVT_H__

