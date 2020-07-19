#include <kernel/irq_types.h>
#include <kernel/serial_types.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <kernel/panic.h>
#include <devices/interrupt-controller/arm-gicv3/arm_gicv3.h>
#include <devices/serial/arm-pl011/arm_pl011.h>
#include <string.h>
#include <platform/platform.h>

arm_gicv3_t arm_gicv3;
arm_pl011_t arm_pl011;

typedef struct {
    int address_cells;
    int size_cells;
} platform_fdt_info_t;

bool _platform_init_serial(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
    // Check if there is a /chosen node with the stdout-path property
    unsigned int node = fdt_get_node(fdth, "/chosen");
    unsigned int prop = fdt_get_prop(fdth, node, "stdout-path");
    unsigned int offset = 0;
    const char *serial_path = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node(fdth, serial_path);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("arm,pl011", compatible) != 0) {
        panic("platform_init_serial: Unsupported serial device");
    }

    // Get the uart base address and size
    prop = fdt_get_prop(fdth, node, "reg");
    offset = 0;
    fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
    uint64_t uart_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->address_cells);
    uint64_t uart_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    // Get the uart base clock frequency
    prop = fdt_get_prop(fdth, node, "clocks");
    offset = 0;
    unsigned int clk_phandle = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node_from_phandle(fdth, clk_phandle);
    prop = fdt_get_prop(fdth, node, "clock-frequency");
    offset = 0;
    uint64_t uart_clock = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    arm_pl011.uart_base = max_kernel_virtual_end + uart_base;
    for (size_t offset = 0; offset < uart_size; offset += PAGESIZE) {
        pmap_kenter_pa(arm_pl011.uart_base + offset, uart_base + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    arm_pl011.uart_clock = uart_clock;
    arm_pl011.baud = ARM_PL011_BAUD_115200;
    arm_pl011.cbits = ARM_PL011_CBITS_8;
    arm_pl011.two_stop_bits = false;
    arm_pl011.even_parity = false;
    arm_pl011.enable_parity = false;

    serial_dev.data = (void*)&arm_pl011;
    serial_dev.ops = &arm_pl011_ops;

    return true;
}

bool _platform_init_irq(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
    unsigned int node = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, node, "interrupt-parent");
    unsigned int offset = 0;
    int intc_phandle = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node_from_phandle(fdth, intc_phandle);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("arm,gic-v3", compatible) != 0) {
        panic("platform_init_irq: Unsupported interrupt controller");
    }

    // Get the GICD base address and size
    prop = fdt_get_prop(fdth, node, "reg");
    fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;
    uint64_t gicd_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->address_cells);
    uint64_t gicd_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    // Get the GICR base address and size
    uint64_t gicr_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->address_cells);
    uint64_t gicr_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    arm_gicv3.gicd_base = max_kernel_virtual_end + gicd_base;
    for (size_t offset = 0; offset < gicd_size; offset += PAGESIZE) {
        pmap_kenter_pa(arm_gicv3.gicd_base + offset, gicd_base + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    arm_gicv3.gicr_base = max_kernel_virtual_end + gicr_base;
    for (size_t offset = 0; offset < gicr_size; offset += PAGESIZE) {
        pmap_kenter_pa(arm_gicv3.gicr_base + offset, gicr_base + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    irq_controller.data = (void*)&arm_gicv3;
    irq_controller.ops = &arm_gicv3_ops;
    irq_controller.spurious_id = ARM_GICV3_SPURIOUS_IRQ_ID;

    // Setup the system timer interrupt
    node = fdt_get_node(fdth, "/timer");
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;

    compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
    if (strcmp("arm,armv8-timer", compatible) != 0 && strcmp("arm,armv7-timer", compatible) != 0) {
        panic("platform_init_irq: Unsupported interrupt controller");
    }

    prop = fdt_get_prop(fdth, node, "interrupts");
    p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;

    int int_type = 0;
    int int_id = 0;
    int int_trigger_type = 0;

    do {
        int_type = fdt_next_data_from_prop(p_prop, &offset);
        int_id = fdt_next_data_from_prop(p_prop, &offset);
        int_trigger_type = fdt_next_data_from_prop(p_prop, &offset);
    } while (int_id != 14);

    // Enable the timer interrupt
    irq_controller.timer_id = ((int_type == 1) ? 16 : 32) + int_id;
    irq_type_t irq_type = int_trigger_type == 4 ? IRQ_TYPE_LEVEL_SENSITIVE : IRQ_TYPE_EDGE_TRIGGERED;
    irq_controller.ops->enable(irq_controller.data, irq_controller.timer_id, 0, irq_type);

    return true;
}

void platform_early_init(fdt_header_t *fdth) {
    // Make sure we're running on the right platform
    unsigned int root = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, root, "compatible");
    unsigned int offset = 0;

    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
    if (strcmp("linux,dummy-virt", compatible) != 0) {
        panic("Wrong platform!");
    }
}

void platform_init(fdt_header_t *fdth) {
    // Get # of address and size cells
    platform_fdt_info_t fdt_info;
    unsigned int root = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, root, "#address-cells");
    unsigned int offset = 0;

    fdt_info.address_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
    prop = fdt_get_prop(fdth, root, "#size-cells");
    offset = 0;
    fdt_info.size_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    _platform_init_serial(fdth, &fdt_info);

    if (!_platform_init_irq(fdth, &fdt_info)) {
        panic("platform: Failed to initialize interrupt controller");
    }
}
