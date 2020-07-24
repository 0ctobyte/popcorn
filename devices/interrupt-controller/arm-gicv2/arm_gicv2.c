/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include "arm_gicv2_dist_if.h"
#include "arm_gicv2_cpu_if.h"
#include "arm_gicv2.h"

#define THIS_PE_NUM           (mpidr_el1_r() & 0xff)
#define IS_SGI(id)            ((id) < 16)
#define IS_PPI(id)            ((id) < 32 && (id) >= 16)
#define IS_SGI_OR_PPI(id)     ((id) < 32)
#define IS_SPI(id)            ((id) < 1020 && (id) >= 32)

irq_controller_dev_ops_t arm_gicv2_ops = {
    .init = arm_gicv2_init,
    .enable = arm_gicv2_enable_irq,
    .disable = arm_gicv2_disable_irq,
    .get_pending = arm_gicv2_get_pending_irq,
    .ack = arm_gicv2_ack_irq,
    .end = arm_gicv2_end_irq,
    .done = arm_gicv2_done_irq,
    .clr = arm_gicv2_clr_irq
};

static inline uint64_t mpidr_el1_r(void) {
    uint64_t v;
    asm ("mrs %0, MPIDR_EL1" : "=r" (v));
    return v;
}

void arm_gicv2_init(void *data) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    // Initialize the distributor and enable non-secure group 1 interrupts
    gicd_ctrl_write(gicv2->gicd_base, S_GICD_CTRL_ENABLE);

    // Set the priority mask and binary point registers. Allow any priority to preempt.
    gicc_pmr_write(gicv2->gicc_base, 0xff);
    gicc_bpr_write(gicv2->gicc_base, 0x0);

    // Enable split EOI mode and non-secure group 1 interrupts
    gicc_ctrl_write(gicv2->gicc_base, S_GICC_CTRL_EOIMODENS | S_GICC_CTRL_ENABLEGRP1);
}

void arm_gicv2_enable_irq(void *data, irq_id_t id, irq_priority_t priority, irq_type_t type) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    // Set the interrupt to group 1
    gicd_igroupr_set(gicv2->gicd_base, id);

    // Set the priority
    gicd_ipriorityr_set(gicv2->gicd_base, id, priority);

    // Set the interrupt type (i.e. level or edge triggered)
    if (type == IRQ_TYPE_EDGE_TRIGGERED) {
        gicd_icfgr_set(gicv2->gicd_base, id);
    } else {
        gicd_icfgr_clr(gicv2->gicd_base, id);
    }

    if (IS_SPI(id)) {
        // Route the interrupt to this CPU
        gicd_itargetsr_set(gicv2->gicd_base, id, 1 << THIS_PE_NUM);
    }

    // Enable it
    gicd_isenabler_set(gicv2->gicd_base, id);
}

void arm_gicv2_disable_irq(void *data, irq_id_t id) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    gicd_icenabler_set(gicv2->gicd_base, id);
}

irq_id_t arm_gicv2_get_pending_irq(void *data) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    return G_GICC_HPPIR_PENDINTID(gicc_hppir_read(gicv2->gicc_base));
}

irq_id_t arm_gicv2_ack_irq(void *data) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    return G_GICC_IAR_INTID(gicc_iar_read(gicv2->gicc_base));
}

void arm_gicv2_end_irq(void *data, irq_id_t id) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    gicc_eoir_write(gicv2->gicc_base, id);
}

void arm_gicv2_done_irq(void *data, irq_id_t id) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    gicc_dir_write(gicv2->gicc_base, id);
}

void arm_gicv2_clr_irq(void *data, irq_id_t id) {
    arm_gicv2_t *gicv2 = (arm_gicv2_t*)data;

    gicd_icpendr_set(gicv2->gicd_base, id);
}
