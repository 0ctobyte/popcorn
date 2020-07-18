#include <kernel/arch/arch_interrupts.h>
#include <kernel/kstdio.h>
#include <kernel/kassert.h>
#include <kernel/irq.h>

irq_controller_dev_t irq_controller;

extern void platform_irq_init(irq_controller_dev_t *irq_controller);

void irq_init(void) {
    platform_irq_init(&irq_controller);

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

kresult_t irq_end(irq_id_t id) {
    if (irq_controller.ops->end == NULL) {
        return KRESULT_OPERATION_NOT_SUPPORTED;
    }

    irq_controller.ops->end(irq_controller.data, id);

    return KRESULT_OK;
}

void irq_clr(irq_id_t id) {
    kassert(irq_controller.ops->clr != NULL);

    irq_controller.ops->clr(irq_controller.data, id);
}

void irq_register_thread(irq_id_t id, struct proc_thread_s *thread) {
    // FIXME Add thread to hash table keyed off ID
}
