/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <string.h>
#include <kernel/panic.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/serial/bcm2835-aux-uart/bcm2835_aux_uart.h>
#include <devices/serial/arm-pl011/arm_pl011.h>
#include <devices/interrupt-controller/arm-gicv2/arm_gicv2.h>
#include <platform/platform.h>

arm_pl011_t arm_pl011;
bcm2835_aux_uart_t bcm2835_aux_uart;
arm_gicv2_t arm_gicv2;

typedef struct {
    uint64_t old_base;
    uint64_t new_base;
    uint64_t size;
} platform_fdt_info_remap_t;

typedef struct {
    int address_cells;
    int soc_address_cells;
    int size_cells;
    platform_fdt_info_remap_t remap[3];
} platform_fdt_info_t;

bool _raspberry_pi_4_platform_init_console(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
    unsigned int alias_node = fdt_get_node(fdth, "/aliases");
    unsigned int prop = fdt_get_prop(fdth, alias_node, "serial0");
    unsigned int offset = 0;
    const char *serial_path = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    unsigned int node = fdt_get_node(fdth, serial_path);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("brcm,bcm2835-aux-uart", compatible) == 0) {
        // The aux miniuart is part of a larger auxillary peripherals block; use the base address of that block
        prop = fdt_get_prop(fdth, alias_node, "aux");
        offset = 0;
        const char *aux_path = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

        unsigned int aux_node = fdt_get_node(fdth, aux_path);
        prop = fdt_get_prop(fdth, aux_node, "reg");
        fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
        offset = 0;
        uint64_t aux_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->soc_address_cells);
        uint64_t aux_size = PAGESIZE;

        for (int i = 0; i < 3; i++) {
            uint64_t old_base = fdt_info->remap[i].old_base;

            if (aux_base >= old_base && aux_base < (old_base + fdt_info->remap[i].size)) {
                aux_base = (aux_base - old_base) + fdt_info->remap[i].new_base;
                break;
            }
        }

        for (size_t offset = 0; offset < aux_size; offset += PAGESIZE) {
            pmap_kenter_pa(max_kernel_virtual_end + aux_base + offset, aux_base + offset, VM_PROT_DEFAULT,
                PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
        }

        bcm2835_aux_uart.uart_base = max_kernel_virtual_end + aux_base;
        bcm2835_aux_uart.cbits = BCM2835_AUX_UART_CBITS_8;
        bcm2835_aux_uart.baud = BCM2835_AUX_UART_BAUD_115200;

        console_dev.data = (void*)&bcm2835_aux_uart;
        console_dev.ops = &bcm2835_aux_uart_ops;

        return true;
    } else if (strcmp("arm,pl011", compatible) == 0) {
        // Get the uart base address and size
        prop = fdt_get_prop(fdth, node, "reg");
        fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
        offset = 0;
        uint64_t uart_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->soc_address_cells);;
        uint64_t uart_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

        for (int i = 0; i < 3; i++) {
            uint64_t old_base = fdt_info->remap[i].old_base;

            if (uart_base >= old_base && uart_base < (old_base + fdt_info->remap[i].size)) {
                uart_base = (uart_base - old_base) + fdt_info->remap[i].new_base;
                break;
            }
        }

        arm_pl011.uart_base = max_kernel_virtual_end + uart_base;
        for (size_t offset = 0; offset < uart_size; offset += PAGESIZE) {
            pmap_kenter_pa(arm_pl011.uart_base + offset, uart_base + offset, VM_PROT_DEFAULT,
                PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
        }

        // FIXME Assuming default UART clock rate of 48MHz; need to mailbox query VC for actual clock rate
        arm_pl011.uart_clock = 48000000;
        arm_pl011.baud = ARM_PL011_BAUD_115200;
        arm_pl011.cbits = ARM_PL011_CBITS_8;
        arm_pl011.two_stop_bits = false;
        arm_pl011.even_parity = false;
        arm_pl011.enable_parity = false;

        console_dev.data = (void*)&arm_pl011;
        console_dev.ops = &arm_pl011_ops;

        return true;
    }

    return false;
}

bool _raspberry_pi_4_platform_init_irq(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
    unsigned int node = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, node, "interrupt-parent");
    unsigned int offset = 0;
    int intc_phandle = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node_from_phandle(fdth, intc_phandle);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("arm,gic-400", compatible) != 0) return false;

    // Get the GICD base address and size
    prop = fdt_get_prop(fdth, node, "reg");
    fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;
    uint64_t gicd_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->soc_address_cells);
    uint64_t gicd_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    for (int i = 0; i < 3; i++) {
        uint64_t old_base = fdt_info->remap[i].old_base;

        if (gicd_base >= old_base && gicd_base < (old_base + fdt_info->remap[i].size)) {
            gicd_base = (gicd_base - old_base) + fdt_info->remap[i].new_base;
            break;
        }
    }

    // Get the GICC base address and size
    uint64_t gicc_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->soc_address_cells);
    uint64_t gicc_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    for (int i = 0; i < 3; i++) {
        uint64_t old_base = fdt_info->remap[i].old_base;

        if (gicc_base >= old_base && gicc_base < (old_base + fdt_info->remap[i].size)) {
            gicc_base = (gicc_base - old_base) + fdt_info->remap[i].new_base;
            break;
        }
    }

    arm_gicv2.gicd_base = max_kernel_virtual_end + gicd_base;
    for (size_t offset = 0; offset < gicd_size; offset += PAGESIZE) {
        pmap_kenter_pa(arm_gicv2.gicd_base + offset, gicd_base + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    arm_gicv2.gicc_base = max_kernel_virtual_end + gicc_base;
    for (size_t offset = 0; offset < gicc_size; offset += PAGESIZE) {
        pmap_kenter_pa(arm_gicv2.gicc_base + offset, gicc_base + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    irq_controller.data = (void*)&arm_gicv2;
    irq_controller.ops = &arm_gicv2_ops;
    irq_controller.spurious_id = ARM_GICV2_SPURIOUS_IRQ_ID;

    // Setup the system timer interrupt
    node = fdt_get_node(fdth, "/timer");
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;

    compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
    if (strcmp("arm,armv8-timer", compatible) != 0 && strcmp("arm,armv7-timer", compatible) != 0) return false;

    prop = fdt_get_prop(fdth, node, "interrupts");
    p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;

    int int_type = 0;
    int int_id = 0;
    int int_trigger_type = 0;

    // The interrupt cells are layed out in this order:
    // Secure timer, non-secure timer, virtual timer and hypervisor timer
    // Get the second interrupt cell for the non-secure physical timer
    for (int i = 0; i < 2; i++) {
        int_type = fdt_next_data_from_prop(p_prop, &offset);
        int_id = fdt_next_data_from_prop(p_prop, &offset);
        int_trigger_type = fdt_next_data_from_prop(p_prop, &offset);
    }


    // Enable the timer interrupt
    irq_controller.timer_id = ((int_type == 1) ? 16 : 32) + int_id;
    irq_type_t irq_type = (int_trigger_type & 0xf) > 2 ? IRQ_TYPE_LEVEL_SENSITIVE : IRQ_TYPE_EDGE_TRIGGERED;
    irq_controller.ops->enable(irq_controller.data, irq_controller.timer_id, 0, irq_type);

    return true;
}

void raspberry_pi_4_platform_init(fdt_header_t *fdth) {
    // Get # of address and size cells
    platform_fdt_info_t fdt_info;
    unsigned int root = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, root, "#address-cells");
    unsigned int offset = 0;

    fdt_info.address_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
    prop = fdt_get_prop(fdth, root, "#size-cells");
    offset = 0;
    fdt_info.size_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    unsigned int node = fdt_get_node(fdth, "/soc");
    prop = fdt_get_prop(fdth, node, "#address-cells");
    offset = 0;
    fdt_info.soc_address_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    prop = fdt_get_prop(fdth, node, "ranges");
    fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;

    int i = 0;
    do {
        fdt_info.remap[i].old_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info.soc_address_cells);
        fdt_info.remap[i].new_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info.address_cells);
        fdt_info.remap[i].size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info.size_cells);
        i++;
    } while (offset != 0);

    _raspberry_pi_4_platform_init_console(fdth, &fdt_info);

    if (!_raspberry_pi_4_platform_init_irq(fdth, &fdt_info)) {
        panic("raspberry_pi_4_platform: Failed to initialize interrupt controller");
    }
}
