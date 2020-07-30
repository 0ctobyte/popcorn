/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _IRQ_H_
#define _IRQ_H_

#include <kernel/kresult.h>
#include <kernel/irq_types.h>

/*
 * irq - This module provides a common generic interface to a variety of interrupt controller devices
 * Each supported platform shall initialize the irq_controller struct with the appropriate device specific data
 * and populate the device function pointers. Not all the functions may be supported by a device. At the minimum init,
 * enable, disable, ack and end shall be supported. Some assumptions made here: the number of priority levels, the
 * number of IRQ IDs and the IRQ "types".
 */

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

// Puts the thread to sleep on the specified IRQ id
kresult_t irq_thread_sleep(struct proc_thread_s *thread, irq_id_t id);

// Generic IRQ handler, this will wake threads waiting on the interrupt that fired
void irq_handler(void);

#endif // _IRQ_H_
