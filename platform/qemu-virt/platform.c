#include <kernel/irq.h>
#include <kernel/fdt.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/interrupt-controller/arm-gicv3/arm_gicv3.h>

#define GICD_BASE (0x08000000)
#define GICD_SIZE (0x10000)
#define GICR_BASE (0x080a0000)
#define GICR_SIZE (0x00f60000)

#define TIMER_INTID_PPI (10 + 16)

arm_gicv3_t arm_gicv3;

irq_controller_dev_ops_t arm_gicv3_ops = {
    .init = arm_gicv3_init,
    .enable = arm_gicv3_enable_irq,
    .disable = arm_gicv3_disable_irq,
    .get_pending = arm_gicv3_get_pending_irq,
    .ack = arm_gicv3_ack_irq,
    .end = arm_gicv3_end_irq,
    .clr = arm_gicv3_clr_irq
};

void platform_irq_init(irq_controller_dev_t *irq_controller) {
    uintptr_t gic_base_va = 0xffffffff00000000;
    arm_gicv3.gicd_base = gic_base_va;

    for (size_t offset = 0; offset < GICD_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(gic_base_va + offset, GICD_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    arm_gicv3.gicr_base = gic_base_va;
    for (size_t offset = 0; offset < GICR_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(gic_base_va + offset, GICR_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    irq_controller->data = (void*)&arm_gicv3;
    irq_controller->ops = &arm_gicv3_ops;
    irq_controller->spurious_id = ARM_GICV3_SPURIOUS_IRQ_ID;
}

void platform_timer_init(void) {
    irq_enable(TIMER_INTID_PPI + 0, 0, IRQ_TYPE_LEVEL_SENSITIVE);
    irq_enable(TIMER_INTID_PPI + 1, 0, IRQ_TYPE_LEVEL_SENSITIVE);
    irq_enable(TIMER_INTID_PPI + 3, 0, IRQ_TYPE_LEVEL_SENSITIVE);
    irq_enable(TIMER_INTID_PPI + 4, 0, IRQ_TYPE_LEVEL_SENSITIVE);
}
