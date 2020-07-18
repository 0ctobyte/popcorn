#ifndef _IRQ_H_
#define _IRQ_H_

#include <sys/types.h>
#include <kernel/kresult.h>

/* irq - This module provides a common generic interface to a variety of interrupt controller devices
 * Each supported platform shall initialize the irq_controller struct with the appropriate device specific data
 * and populate the device function pointers. Not all the functions may be supported by a device. At the minimum init,
 * enable, disable, ack and end shall be supported. Some assumptions made here: the number of priority levels, the
 * number of IRQ IDs and the IRQ "types".
 */

#include <kernel/irq_types.h>

typedef struct {
    // Initializes the IRQ controller
    void (*init)(void*);

    // Enables the given IRQ ID with the given priority and type
    void (*enable)(void*, irq_id_t, irq_priority_t, irq_type_t);

    // Disables the given IRQ ID. The kernel should not expect interrupts from that ID anymore
    void (*disable)(void*, irq_id_t);

    // Get's the highest priority pending IRQ ID without acknowledging it
    irq_id_t (*get_pending)(void*);

    // Acknowledges a pending interrupt, this shall return the IRQ ID of the highest pending interrupt
    irq_id_t (*ack)(void*);

    // Ends the pending interrupt. This should be matched with a prior ack call
    // This is typically used to signal the interrupt controller that the interrupt has been minimally handled inside
    // the kernel. A device driver thread may need to take over handling the interrupt further.
    void (*end)(void*, irq_id_t);

    // Indicates the IRQ has been handled and is no longer active or pending
    // This is typically used after a device driver thread has handled the interrupt from the device side
    void (*done)(void*, irq_id_t);

    // Clears the pending interrupt
    void (*clr)(void*, irq_id_t);
} irq_controller_dev_ops_t;

typedef struct {
    void *data;                    // Device specific data
    irq_id_t spurious_id;          // The ID of the spurious interrupt
    irq_controller_dev_ops_t *ops; // Device operations
} irq_controller_dev_t;

#define IRQ_SPURIOUS_ID (irq_controller.spurious_id)

struct proc_thread_s;

// Kernel interface
void irq_init(void);
void irq_enable(irq_id_t id, irq_priority_t priority, irq_type_t type);
void irq_disable(irq_id_t id);
kresult_t irq_get_pending(irq_id_t *id);
irq_id_t irq_ack(void);
void irq_end(irq_id_t id);
kresult_t irq_done(irq_id_t id);
kresult_t irq_clr(irq_id_t id);

// Register the given IRQ thread to be scheduled whenever the given IRQ ID fires
void irq_register_thread(irq_id_t id, struct proc_thread_s *thread);

// Generic IRQ handler
void irq_handler(void);

#endif // _IRQ_H_
