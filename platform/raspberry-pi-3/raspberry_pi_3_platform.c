#include <string.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/serial/bcm2385-aux-uart/bcm2385_aux_uart.h>
#include <platform/platform.h>

//#define R_UART0_BASE (0x3F201000) // Raspberry Pi 3 UART0

bcm2385_aux_uart_t bcm2385_aux_uart;

typedef struct {
    int address_cells;
    int size_cells;
    uint64_t remap[3];
} platform_fdt_info_t;

bool _raspberry_pi_3_platform_init_console(fdt_header_t *fdth, platform_fdt_info_t *fdt_info) {
    // Map the aux IO block which contains the aux-uart
    unsigned int node = fdt_get_node(fdth, "/aliases");
    unsigned int prop = fdt_get_prop(fdth, node, "aux");
    unsigned int offset = 0;
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

    prop = fdt_get_prop(fdth, node, "serial0");
    offset = 0;
    const char *serial_path = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    node = fdt_get_node(fdth, serial_path);
    prop = fdt_get_prop(fdth, node, "compatible");
    offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    if (strcmp("brcm,bcm2835-aux-uart", compatible) != 0) return false;

    // The aux miniuart is part of a larger auxillary peripherals block; use the base address of that block
    bcm2385_aux_uart.uart_base = max_kernel_virtual_end + aux_base;
    bcm2385_aux_uart.cbits = BCM2385_AUX_UART_CBITS_8;
    bcm2385_aux_uart.baud = BCM2385_AUX_UART_BAUD_115200;

    console_dev.data = (void*)&bcm2385_aux_uart;
    console_dev.ops = &bcm2385_aux_uart_ops;
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
}
