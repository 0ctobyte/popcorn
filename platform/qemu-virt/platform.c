#include <kernel/irq_types.h>
#include <kernel/serial_types.h>
#include <kernel/fdt.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/interrupt-controller/arm-gicv3/arm_gicv3.h>
#include <devices/serial/arm-pl011/arm_pl011.h>
#include <platform/platform.h>

#define UART0_BASE (0x09000000)
#define UART0_SIZE (0x1000)

#define GICD_BASE (0x08000000)
#define GICD_SIZE (0x10000)
#define GICR_BASE (0x080a0000)
#define GICR_SIZE (0x00f60000)

#define TIMER_INTID_PPI (14 + 16)

arm_gicv3_t arm_gicv3;
arm_pl011_t arm_pl011;

void platform_early_init(void) {
}

void platform_init(void) {
    arm_pl011.uart_base = max_kernel_virtual_end + UART0_BASE;
    for (size_t offset = 0; offset < UART0_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(arm_pl011.uart_base + offset, UART0_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    serial_dev.data = (void*)&arm_pl011;
    serial_dev.ops = &arm_pl011_ops;

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

    irq_controller.data = (void*)&arm_gicv3;
    irq_controller.ops = &arm_gicv3_ops;
    irq_controller.spurious_id = ARM_GICV3_SPURIOUS_IRQ_ID;

    // Enable the timer interrupt
    irq_controller.ops->enable(irq_controller.data, TIMER_INTID_PPI, 0, IRQ_TYPE_LEVEL_SENSITIVE);
}
