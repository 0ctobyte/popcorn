#include <kernel/irq.h>
#include <kernel/fdt.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/interrupt-controller/arm-gicv3/arm_gicv3.h>

#define GICD_BASE (0x08000000)
#define GICD_SIZE (0x10000)
#define GICR_BASE (0x080a0000)
#define GICR_SIZE (0x00f60000)

#define TIMER_INTID_PPI (14 + 16)

arm_gicv3_t arm_gicv3;

void platform_irq_init(irq_controller_dev_t *irq_controller) {
    arm_gicv3.gicd_base = max_kernel_virtual_end + GICD_BASE;
    for (size_t offset = 0; offset < GICD_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(arm_gicv3.gicd_base + offset, GICD_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    arm_gicv3.gicr_base = max_kernel_virtual_end + GICR_BASE;
    for (size_t offset = 0; offset < GICR_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(arm_gicv3.gicr_base + offset, GICR_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    irq_controller->data = (void*)&arm_gicv3;
    irq_controller->ops = &arm_gicv3_ops;
    irq_controller->spurious_id = ARM_GICV3_SPURIOUS_IRQ_ID;
}

void platform_timer_init(void) {
    irq_enable(TIMER_INTID_PPI, 0, IRQ_TYPE_LEVEL_SENSITIVE);
}
