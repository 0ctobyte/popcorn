/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _ARM_GICV2_H_
#define _ARM_GICV2_H_

#include <sys/types.h>
#include <kernel/irq_types.h>

typedef struct {
    uintptr_t gicd_base;
    uintptr_t gicc_base;
} arm_gicv2_t;

extern irq_controller_dev_ops_t arm_gicv2_ops;

#define ARM_GICV2_SPURIOUS_IRQ_ID (1023)

void arm_gicv2_init(void *data);
void arm_gicv2_enable_irq(void *data, irq_id_t id, irq_priority_t priority, irq_type_t type);
void arm_gicv2_disable_irq(void  *data, irq_id_t id);
irq_id_t arm_gicv2_get_pending_irq(void *data);
irq_id_t arm_gicv2_ack_irq(void *data);
void arm_gicv2_end_irq(void *data, irq_id_t id);
void arm_gicv2_done_irq(void *data, irq_id_t id);
void arm_gicv2_clr_irq(void *data, irq_id_t id);

#endif // _ARM_GICV2_H_
