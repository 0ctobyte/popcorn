/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _BCM2836_L1_INTC_H_
#define _BCM2836_L1_INTC_H_

#include <sys/types.h>
#include <kernel/irq_types.h>

typedef struct {
    uintptr_t intc_base;
    uintptr_t intc_armctrl_base;
} bcm2836_l1_intc_t;

extern irq_controller_dev_ops_t bcm2836_l1_intc_ops;

#define BCM2836_L1_INTC_SPURIOUS_IRQ_ID (255)

void bcm2836_l1_intc_init(void *data);
void bcm2836_l1_intc_enable_irq(void *data, irq_id_t id, irq_priority_t priority, irq_type_t type);
void bcm2836_l1_intc_disable_irq(void  *data, irq_id_t id);
irq_id_t bcm2836_l1_intc_get_pending_irq(void *data);
irq_id_t bcm2836_l1_intc_ack_irq(void *data);
void bcm2836_l1_intc_end_irq(void *data, irq_id_t id);
void bcm2836_l1_intc_done_irq(void *data, irq_id_t id);
void bcm2836_l1_intc_clr_irq(void *data, irq_id_t id);

#endif // _BCM2836_L1_INTC_H_
