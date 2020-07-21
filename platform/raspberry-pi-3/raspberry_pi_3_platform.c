#include <string.h>
#include <kernel/panic.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/serial/bcm2835-aux-uart/bcm2835_aux_uart.h>
#include <devices/serial/arm-pl011/arm_pl011.h>
#include <devices/interrupt-controller/bcm2836-l1-intc/bcm2836_l1_intc.h>
#include <platform/platform.h>

arm_pl011_t arm_pl011;
bcm2835_aux_uart_t bcm2835_aux_uart;
bcm2836_l1_intc_t bcm2836_l1_intc;

typedef struct {
    int address_cells;
    int size_cells;
    uint64_t remap[3];
} platform_fdt_info_t;

bool _raspberry_pi_3_platform_init_console(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
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
        uint64_t aux_base = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
        aux_base = (aux_base - fdt_info->remap[0]) + fdt_info->remap[1];
        uint64_t aux_size = PAGESIZE;

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
        uint64_t uart_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->address_cells);
        uart_base = (uart_base - fdt_info->remap[0]) + fdt_info->remap[1];
        uint64_t uart_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

        arm_pl011.uart_base = max_kernel_virtual_end + uart_base;
        for (size_t offset = 0; offset < uart_size; offset += PAGESIZE) {
            pmap_kenter_pa(arm_pl011.uart_base + offset, uart_base + offset, VM_PROT_DEFAULT,
                PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
        }

        // FIXME Assuming default UART clock rate of 3MHz; need to mailbox query VC for actual clock rate
        arm_pl011.uart_clock = 3000000;
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

bool _raspberry_pi_3_platform_init_irq(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
    // Find the global interrupt controller
    unsigned int node = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, node, "interrupt-parent");
    unsigned int offset = 0;
    int intc_phandle = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node_from_phandle(fdth, intc_phandle);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("brcm,bcm2836-armctrl-ic", compatible) != 0) return false;

    prop = fdt_get_prop(fdth, node, "reg");
    fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;
    uint64_t intc_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->address_cells);
    intc_base = (intc_base - fdt_info->remap[0]) + fdt_info->remap[1];
    uint64_t intc_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    bcm2836_l1_intc.intc_armctrl_base = max_kernel_virtual_end + intc_base;
    for (size_t offset = 0; offset < intc_size; offset += PAGESIZE) {
        uintptr_t va = ROUND_PAGE_DOWN(bcm2836_l1_intc.intc_armctrl_base + offset);
        uintptr_t pa = ROUND_PAGE_DOWN(intc_base + offset);

        pmap_kenter_pa(va, pa, VM_PROT_DEFAULT, PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    // Find the local interrupt-controller
    prop = fdt_get_prop(fdth, node, "interrupt-parent");
    offset = 0;
    intc_phandle = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node_from_phandle(fdth, intc_phandle);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("brcm,bcm2836-l1-intc", compatible) != 0) return false;

    // Get the base address and size
    prop = fdt_get_prop(fdth, node, "reg");
    p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;
    intc_base = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->address_cells);
    intc_size = fdt_next_data_cells_from_prop(p_prop, &offset, fdt_info->size_cells);

    bcm2836_l1_intc.intc_base = max_kernel_virtual_end + intc_base;
    for (size_t offset = 0; offset < intc_size; offset += PAGESIZE) {
        pmap_kenter_pa(bcm2836_l1_intc.intc_base + offset, intc_base + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    irq_controller.data = (void*)&bcm2836_l1_intc;
    irq_controller.ops = &bcm2836_l1_intc_ops;
    irq_controller.spurious_id = BCM2836_L1_INTC_SPURIOUS_IRQ_ID;

    // Setup the system timer interrupt
    node = fdt_get_node(fdth, "/timer");
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;

    compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);
    if (strcmp("arm,armv8-timer", compatible) != 0 && strcmp("arm,armv7-timer", compatible) != 0) return false;

    prop = fdt_get_prop(fdth, node, "interrupts");
    p_prop = fdt_get_prop_from_offset(fdth, prop);
    offset = 0;

    int int_id = 0;
    int int_trigger_type = 0;

    // The interrupt cells are layed out in this order:
    // Secure timer, non-secure timer, virtual timer and hypervisor timer
    // Get the third interrupt cell for the non-secure physical timer
    for (int i = 0; i < 2; i++) {
        int_id = fdt_next_data_from_prop(p_prop, &offset);
        int_trigger_type = fdt_next_data_from_prop(p_prop, &offset);
    }

    // Enable the timer interrupt
    irq_controller.timer_id = int_id;
    irq_type_t irq_type = int_trigger_type == 4 ? IRQ_TYPE_LEVEL_SENSITIVE : IRQ_TYPE_EDGE_TRIGGERED;
    irq_controller.ops->enable(irq_controller.data, irq_controller.timer_id, 0, irq_type);

    return true;
}

void raspberry_pi_3_platform_init(fdt_header_t *fdth) {
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
    prop = fdt_get_prop(fdth, node, "ranges");
    offset = 0;

    fdt_prop_t *p_prop = fdt_get_prop_from_offset(fdth, prop);
    fdt_info.remap[0] = fdt_next_data_from_prop(p_prop, &offset);
    fdt_info.remap[1] = fdt_next_data_from_prop(p_prop, &offset);
    fdt_info.remap[2] = fdt_next_data_from_prop(p_prop, &offset);

    _raspberry_pi_3_platform_init_console(fdth, &fdt_info);

    if (!_raspberry_pi_3_platform_init_irq(fdth, &fdt_info)) {
        panic("raspberry_pi_3_platform: Failed to initialize interrupt controller");
    }
}
