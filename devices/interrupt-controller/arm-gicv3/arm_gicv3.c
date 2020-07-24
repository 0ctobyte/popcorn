/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include "arm_gicv3_dist_if.h"
#include "arm_gicv3_rdist_if.h"
#include "arm_gicv3_cpu_if.h"
#include "arm_gicv3.h"

#define THIS_PE_NUM           (mpidr_el1_r() & 0xff)
#define GICR_PE_OFFSET(g, pe) (g->gicr_base + pe * 0x20000)
#define IS_SGI(id)            ((id) < 16)
#define IS_PPI(id)            ((id) < 32 && (id) >= 16)
#define IS_SGI_OR_PPI(id)     ((id) < 32)
#define IS_SPI(id)            ((id) < 1020 && (id) >= 32)

irq_controller_dev_ops_t arm_gicv3_ops = {
    .init = arm_gicv3_init,
    .enable = arm_gicv3_enable_irq,
    .disable = arm_gicv3_disable_irq,
    .get_pending = arm_gicv3_get_pending_irq,
    .ack = arm_gicv3_ack_irq,
    .end = arm_gicv3_end_irq,
    .done = arm_gicv3_done_irq,
    .clr = arm_gicv3_clr_irq
};

static inline uint64_t mpidr_el1_r(void) {
    uint64_t v;
    asm ("mrs %0, MPIDR_EL1" : "=r" (v));
    return v;
}

void arm_gicv3_init(void *data) {
    arm_gicv3_t *gicv3 = (arm_gicv3_t*)data;

    uintptr_t gicr_base = GICR_PE_OFFSET(gicv3, THIS_PE_NUM);

    // Initialize the distributor and enable non-secure group 1 interrupts
    gicd_ctrl_write(gicv3->gicd_base, S_GICD_CTRL_ARE | S_GICD_CTRL_ENABLEGRP1);

    // Mark this processor as awake in the redistributor
    uint32_t waker = gicr_waker_read(gicr_base);
    kassert(G_GICR_WAKER_CHILDRENASLEEP(waker));
    gicr_waker_write(gicr_base, 0);
    do {
        waker = gicr_waker_read(gicr_base);
    } while (G_GICR_WAKER_CHILDRENASLEEP(waker));

    // Enable system register access
    icc_sre_el1_w(1);
    asm ("isb sy");

    // Set the priority mask and binary point registers. Allow any priority to preempt.
    icc_pmr_el1_w(0xff);
    icc_bpr1_el1_w(0x0);

    // Enable split EOI mode
    icc_ctrl_el1_w(S_ICC_CTRL_EOIMODE);

    // Enable non-secure group 1 interrupts
    icc_igrpen1_el1_w(1);
    asm ("isb sy");
}

void arm_gicv3_enable_irq(void *data, irq_id_t id, irq_priority_t priority, irq_type_t type) {
    arm_gicv3_t *gicv3 = (arm_gicv3_t*)data;

    uintptr_t gicr_base = GICR_PE_OFFSET(gicv3, THIS_PE_NUM);

    if (IS_SGI_OR_PPI(id)) {
        // Set the interrupt to group 1
        gicr_igroupr_set(gicr_base, id);

        // Set the priority
        gicr_ipriorityr_set(gicr_base, id, priority);

        // Set the interrupt type (i.e. level or edge triggered)
        if (type == IRQ_TYPE_EDGE_TRIGGERED) {
            gicr_icfgr_set(gicr_base, id);
        } else {
            gicr_icfgr_clr(gicr_base, id);
        }

        // Enable it
        gicr_isenabler_set(gicr_base, id);
    } else {
        // Set the interrupt to group 1
        gicd_igroupr_set(gicv3->gicd_base, id);

        // Set the priority
        gicd_ipriorityr_set(gicv3->gicd_base, id, priority);

        // Set the interrupt type (i.e. level or edge triggered)
        if (type == IRQ_TYPE_EDGE_TRIGGERED) {
            gicd_icfgr_set(gicv3->gicd_base, id);
        } else {
            gicd_icfgr_clr(gicv3->gicd_base, id);
        }

        // Route the interrupt to this CPU
        gicd_irouter_write(gicv3->gicd_base, id, mpidr_el1_r() & ~S_GICD_IROUTER_INTERRUPT_ROUTING_MODE);

        // Enable it
        gicd_isenabler_set(gicr_base, id);
    }
}

void arm_gicv3_disable_irq(void *data, irq_id_t id) {
    arm_gicv3_t *gicv3 = (arm_gicv3_t*)data;

    uintptr_t gicr_base = GICR_PE_OFFSET(gicv3, THIS_PE_NUM);

    if (IS_SGI_OR_PPI(id)) {
        gicr_icenabler_set(gicr_base, id);
    } else {
        gicd_icenabler_set(gicv3->gicd_base, id);
    }
}

irq_id_t arm_gicv3_get_pending_irq(void *data) {
    return icc_hppir1_el1_r();
}

irq_id_t arm_gicv3_ack_irq(void *data) {
    return icc_iar1_el1_r();
}

void arm_gicv3_end_irq(void *data, irq_id_t id) {
    icc_eoir1_el1_w(id);
}

void arm_gicv3_done_irq(void *data, irq_id_t id) {
    icc_dir_el1_w(id);
}

void arm_gicv3_clr_irq(void *data, irq_id_t id) {
    arm_gicv3_t *gicv3 = (arm_gicv3_t*)data;

    uintptr_t gicr_base = GICR_PE_OFFSET(gicv3, THIS_PE_NUM);

    if (IS_SGI_OR_PPI(id)) {
        gicr_icpendr_set(gicr_base, id);
    } else {
        gicd_icpendr_set(gicr_base, id);
    }
}
