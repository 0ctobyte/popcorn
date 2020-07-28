/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _FDT_H_
#define _FDT_H_

#include <sys/types.h>

/*
 * fdt - Flattened device tree parsing support
 * Supports iterating and searching for nodes and retrieving properties from nodes. It also allows iterating through
 * property data and lists.
 */

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

// Checks whether the property data is a string
bool fdt_prop_data_is_string(fdt_prop_t *prop);

// Get the name string/len of the given property
const char* fdt_get_name_from_prop(fdt_header_t *fdth, fdt_prop_t *prop);
uint32_t fdt_get_len_from_prop(fdt_prop_t *prop);

// Iterate through the data for a property. The iterators can be started by setting data_offset to 0
uint32_t fdt_next_data_from_prop(fdt_prop_t *prop, unsigned int *data_offset);
const char* fdt_next_string_from_prop(fdt_prop_t *prop, unsigned int *string_offset);

// Iterate through the property data returning a group of cells specified by num_cells rather than one cell at at time
// The return value is 64-bits so this function only returns the last 2 cells of the data value
uint64_t fdt_next_data_cells_from_prop(fdt_prop_t *prop, unsigned int *data_offset, unsigned int num_cells);

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
unsigned int fdt_get_node(fdt_header_t *fdth, const char *path);

// Searches for the node with the matching phandle
unsigned int fdt_get_node_from_phandle(fdt_header_t *fdth, int phandle);

// Returns the offset to the root node
unsigned int fdt_get_root_node(fdt_header_t *fdth);

// Returns the offset to the very first node in the FDT (other than the root node)
unsigned int fdt_get_first_node(fdt_header_t *fdth);

// Search for the property with the specified name given the offset to a node
unsigned int fdt_get_prop(fdt_header_t *fdth, unsigned int offset, const char *name);

// Dump just the FDT in a human readable format
void fdt_dump_header(fdt_header_t *fdth);
void fdt_dump(fdt_header_t *fdth);

#endif // _FDT_H_
