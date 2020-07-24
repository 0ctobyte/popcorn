/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _IRQ_TYPES_H_
#define _IRQ_TYPES_H_

#include <sys/types.h>

typedef uint8_t irq_priority_t;
typedef uint16_t irq_id_t;

typedef enum {
    IRQ_TYPE_LEVEL_SENSITIVE,
    IRQ_TYPE_EDGE_TRIGGERED,
} irq_type_t;

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
    irq_id_t timer_id;             // The timer interrupt ID
    irq_controller_dev_ops_t *ops; // Device operations
} irq_controller_dev_t;

#define IRQ_SPURIOUS_ID (irq_controller.spurious_id)
#define IRQ_TIMER_ID    (irq_controller.timer_id)

// Initialized by platform code
extern irq_controller_dev_t irq_controller;

#endif // _IRQ_TYPES_H_
