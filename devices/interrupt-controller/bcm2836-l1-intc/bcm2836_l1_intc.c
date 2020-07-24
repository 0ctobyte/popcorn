/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/arch/arch_asm.h>
#include "bcm2836_l1_intc_if.h"
#include "bcm2836_l1_intc.h"

irq_controller_dev_ops_t bcm2836_l1_intc_ops = {
    .init = bcm2836_l1_intc_init,
    .enable = bcm2836_l1_intc_enable_irq,
    .disable = bcm2836_l1_intc_disable_irq,
    .get_pending = bcm2836_l1_intc_get_pending_irq,
    .ack = bcm2836_l1_intc_ack_irq,
    .end = bcm2836_l1_intc_end_irq,
    .done = bcm2836_l1_intc_done_irq,
    .clr = bcm2836_l1_intc_clr_irq
};

static inline uint64_t mpidr_el1_r(void) {
    uint64_t v;
    asm ("mrs %0, MPIDR_EL1" : "=r" (v));
    return v;
}

void bcm2836_l1_intc_init(void *data) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
    unsigned int core = mpidr_el1_r() & 0xff;

    // Clear interrupt sources; route GPU interrupts to core0
    intc_control_write(intc->intc_base, 0);
    intc_gpu_int_routing_write(intc->intc_base, 0);
    intc_pmu_int_routing_clr_write(intc->intc_base, 0xffffffff);
    intc_axi_interrupts_write(intc->intc_base, 0);
}

void bcm2836_l1_intc_enable_irq(void *data, irq_id_t id, irq_priority_t priority, irq_type_t type) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
    unsigned int core = mpidr_el1_r() & 0xff;

    // FIXME no support for GPU and PMU interrupts
    if (id >= 8) return;

    if (id < 4) {
        // Timer ID
        uint32_t timer_ctrl = intc_core_timer_ctrl_read(intc->intc_base, core);
        intc_core_timer_ctrl_write(intc->intc_base, core, timer_ctrl | (1 << id));
    } else {
        // Mailbox ID
        uint32_t mailbox_ctrl = intc_core_mailbox_ctrl_read(intc->intc_base, core);
        intc_core_mailbox_ctrl_write(intc->intc_base, core, mailbox_ctrl | (1 << id));
    }
}

void bcm2836_l1_intc_disable_irq(void *data, irq_id_t id) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
    unsigned int core = mpidr_el1_r() & 0xff;

    // FIXME no support for GPU and PMU interrupts
    if (id >= 8) return;

    if (id < 4) {
        // Timer ID
        uint32_t timer_ctrl = intc_core_timer_ctrl_read(intc->intc_base, core);
        intc_core_timer_ctrl_write(intc->intc_base, core, timer_ctrl & ~(1 << id));
    } else {
        // Mailbox ID
        uint32_t mailbox_ctrl = intc_core_mailbox_ctrl_read(intc->intc_base, core);
        intc_core_mailbox_ctrl_write(intc->intc_base, core, mailbox_ctrl & ~(1 << id));
    }
}

irq_id_t bcm2836_l1_intc_get_pending_irq(void *data) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
    unsigned int core = mpidr_el1_r() & 0xff;

    irq_id_t id = intc_core_interrupt_source_read(intc->intc_base, core);
    id = arch_ctz(id & ~(id - 1));

    return id;
}

irq_id_t bcm2836_l1_intc_ack_irq(void *data) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
    unsigned int core = mpidr_el1_r() & 0xff;

    irq_id_t id = intc_core_interrupt_source_read(intc->intc_base, core);
    id = arch_ctz(id & ~(id - 1));

    return id;
}

void bcm2836_l1_intc_end_irq(void *data, irq_id_t id) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
}

void bcm2836_l1_intc_done_irq(void *data, irq_id_t id) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
}

void bcm2836_l1_intc_clr_irq(void *data, irq_id_t id) {
    bcm2836_l1_intc_t *intc = (bcm2836_l1_intc_t*)data;
}
