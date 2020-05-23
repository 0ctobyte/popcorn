#ifndef __FDT_H__
#define __FDT_H__

// Flattened Device Tree parsing support
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t fdt_token_t;

typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header_t;

typedef struct {
    uint64_t address;
    uint64_t size;
} fdt_reserve_entry_t;

typedef struct {
    fdt_token_t token;
    const char name[];
} fdt_node_t;

typedef struct {
    fdt_token_t token;
    uint32_t len;
    uint32_t nameoff;
    uint8_t data[];
} fdt_prop_t;

extern fdt_header_t *fdt_header;

// Checks whether the property data is a string
bool fdt_prop_data_is_string(fdt_prop_t *prop);

// Get the name string of the given property
const char* fdt_get_name_from_prop(fdt_header_t *fdth, fdt_prop_t *prop);

// Get's the node/prop/rsv at the specified offset
fdt_node_t* fdt_get_node_from_offset(fdt_header_t *fdth, unsigned int offset);
fdt_prop_t* fdt_get_prop_from_offset(fdt_header_t *fdth, unsigned int offset);
fdt_reserve_entry_t* fdt_get_rsv_from_offset(fdt_header_t *fdth, unsigned int offset);

// Returns the offset to the next node/subnode/prop given the offset of the previous node
// Returns 0 if there is no more nodes/subnodes/props
unsigned int fdt_next_node(fdt_header_t *fdth, unsigned int offset);
unsigned int fdt_next_subnode(fdt_header_t *fdth, unsigned int offset);
unsigned int fdt_next_prop(fdt_header_t *fdth, unsigned int offset);

// Search for the node with the specified path. If unit addresses are provided in the path it will search
// for the unique node otherwise it will stop after the finding the first matching node
unsigned int fdt_get_node(fdt_header_t *fdth, const char *name);

// Returns the offset to the very first node in the FDT (other than the root node)
unsigned int fdt_get_first_node(fdt_header_t *fdth);

// Search for the property with the specified given the offset to a node
unsigned int fdt_get_prop(fdt_header_t *fdth, unsigned int offset, const char *name);

// Dump just the FDT in a human readable format
void fdt_dump_header(fdt_header_t *fdth);
void fdt_dump(fdt_header_t *fdth);

#endif // __FDT_H__
