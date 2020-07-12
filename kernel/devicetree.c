#include <kernel/fdt.h>
#include <kernel/devicetree.h>

extern fdt_header_t *fdt_header;

#include <kernel/kstdio.h>
bool devicetree_find_memory(unsigned long *base_addr, unsigned long *size) {
    // Get the size and address cells
    unsigned int root_offset = fdt_get_root_node(fdt_header), data_offset = 0;
    unsigned int address_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdt_header,
        fdt_get_prop(fdt_header, root_offset, "#address-cells")), &data_offset);
    unsigned int size_cells = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdt_header,
        fdt_get_prop(fdt_header, root_offset, "#size-cells")), &data_offset);

    // Search for the memory node and get the reg prop
    unsigned int node_offset = fdt_get_node(fdt_header, "/memory");
    if (node_offset == 0) return false;
    unsigned int prop_offset = fdt_get_prop(fdt_header, node_offset, "reg");
    if (prop_offset == 0) return false;

    // The reg field is arranges as #address_cells for base address followed by #size_cells for memory size
    fdt_prop_t *prop = fdt_get_prop_from_offset(fdt_header, prop_offset);

    data_offset = 0;
    unsigned long mem_base_addr = 0;
    for (unsigned int i = 0; i < address_cells; i++) {
        mem_base_addr = (mem_base_addr << 32) | fdt_next_data_from_prop(prop, &data_offset);
    }

    if (data_offset == 0) return false;

    unsigned long mem_size = 0;
    for (unsigned int i = 0; i < size_cells; i++) {
        mem_size = (mem_size << 32) | fdt_next_data_from_prop(prop, &data_offset);
    }

    *base_addr = mem_base_addr;
    *size = mem_size;

    return true;
}

bool devicetree_find_uart(void) {
}
