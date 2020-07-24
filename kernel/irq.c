/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/arch/arch_interrupts.h>
#include <kernel/arch/arch_timer.h>
#include <kernel/proc/proc_scheduler.h>
#include <kernel/irq.h>

irq_controller_dev_t irq_controller;

void irq_init(void) {
    // The irq_controller data structure should be intialized by platform code earlier
    kassert(irq_controller.ops != NULL && irq_controller.ops->init != NULL);
    irq_controller.ops->init(irq_controller.data);

    arch_interrupts_enable();
}

void irq_enable(irq_id_t id, irq_priority_t priority, irq_type_t type) {
    kassert(irq_controller.ops->enable != NULL);

    irq_controller.ops->enable(irq_controller.data, id, priority, type);
}

void irq_disable(irq_id_t id) {
    kassert(irq_controller.ops->disable != NULL);

    irq_controller.ops->disable(irq_controller.data, id);
}

kresult_t irq_get_pending(irq_id_t *id) {
    if (irq_controller.ops->get_pending == NULL) {
        return KRESULT_OPERATION_NOT_SUPPORTED;
    }

    *id = irq_controller.ops->get_pending(irq_controller.data);

    return KRESULT_OK;
}

irq_id_t irq_ack(void) {
    kassert(irq_controller.ops->ack != NULL);

    return irq_controller.ops->ack(irq_controller.data);
}

void irq_end(irq_id_t id) {
    kassert(irq_controller.ops->end != NULL);

    irq_controller.ops->end(irq_controller.data, id);
}

kresult_t irq_done(irq_id_t id) {
    if (irq_controller.ops->done == NULL) {
        return KRESULT_OPERATION_NOT_SUPPORTED;
    }

    irq_controller.ops->done(irq_controller.data, id);

    return KRESULT_OK;
}

kresult_t irq_clr(irq_id_t id) {
    if (irq_controller.ops->clr == NULL) {
        return KRESULT_OPERATION_NOT_SUPPORTED;
    }

    irq_controller.ops->clr(irq_controller.data, id);

    return KRESULT_OK;
}

kresult_t irq_thread_sleep(struct proc_thread_s *thread, irq_id_t id) {
    // FIXME Add the thread to the event hash table; event is (irq_controller.data << 16) | id
    return KRESULT_UNIMPLEMENTED;
}

void irq_handler(void) {
    irq_id_t id = irq_ack();

    // Ignore spurious interrupts for now
    if (id == IRQ_SPURIOUS_ID) return;

    // FIXME Wake thread from hash table to run with interrupt priority
    arch_timer_stop();

    irq_end(id);
    irq_done(id);

    arch_timer_start_msecs(5000);
    proc_scheduler_choose();
}
