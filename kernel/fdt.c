/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <ctype.h>
#include <kernel/kstdio.h>
#include <kernel/panic.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/fdt.h>

#define FDT_MAGIC (0xd00dfeed)

#define FDT_INVALID    (0x00000000)
#define FDT_BEGIN_NODE (0x00000001) // Marks the beginning of a node's representation
#define FDT_END_NODE   (0x00000002) // Marks the end of a node's representation
#define FDT_PROP       (0x00000003) // Marks the beginning of the representation of one property
#define FDT_NOP        (0x00000004) // Skipped by parser
#define FDT_END        (0x00000009) // Marks the end of the structure block.

fdt_token_t _fdt_get_token_from_offset(fdt_header_t *fdth, unsigned int offset) {
    return arch_rev32(*(fdt_token_t*)((uintptr_t)fdth + offset));
}

fdt_token_t _fdt_next_token(fdt_header_t *fdth, unsigned int *offset) {
    uintptr_t addr = (uintptr_t)fdth + *offset;
    fdt_token_t token = _fdt_get_token_from_offset(fdth, *offset);

    switch (token) {
        case FDT_BEGIN_NODE:
        {
            // Skip name
            fdt_node_t *node = (fdt_node_t*)addr;
            size_t len = (strlen(node->name) + 4 & ~0x3);
            *offset += sizeof(fdt_node_t) + len;
            break;
        }
        case FDT_END_NODE:
        {
            // For FDT_END_NODE just go to the next word
            *offset += sizeof(fdt_token_t);
            break;
        }
        case FDT_PROP:
        {
            // Skip the prop header and data
            fdt_prop_t *prop = (fdt_prop_t*)addr;
            size_t len = (fdt_get_len_from_prop(prop) + 3) & ~0x3;
            *offset += sizeof(fdt_prop_t) + len;
            break;
        }
        case FDT_NOP:
        {
            // Skip all NOPs
            for (fdt_token_t *next_token = (fdt_token_t*)addr + 1; arch_rev32(*next_token) == FDT_NOP; next_token++);
            break;
        }
        default: return 0;
    }

    return arch_rev32(*(fdt_token_t*)((uintptr_t)fdth + *offset));
}

unsigned int _fdt_find_node_from_offset(fdt_header_t *fdth, unsigned int offset, const char *name, size_t num) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;

    for (; offset != 0; offset = fdt_next_node(fdth, offset)) {
        fdt_node_t *node = fdt_get_node_from_offset(fdth, offset);
        if (strncmp(name, node->name, num) == 0) break;
    }

    return offset;
}

bool fdt_prop_data_is_string(fdt_prop_t *prop) {
    uint32_t len = fdt_get_len_from_prop(prop);
    const char *str = (const char*)prop->data;

    // Strings must be null-terminated and greater than 0 in length
    if (len == 0 || str[len - 1] != '\0') return false;

    unsigned int num_chars = 0, num_nulls = 0;
    for (unsigned int i = 0; i < len; i++) {
        if (str[i] == '\0') {
            num_nulls += 1;
        } else if (isprint(str[i])) {
            num_chars += 1;
        } else {
            return false;
        }
    }

    return len == (num_chars + num_nulls) && num_chars != 0 && num_chars > num_nulls;
}

fdt_node_t* fdt_get_node_from_offset(fdt_header_t *fdth, unsigned int offset) {
    return (fdt_node_t*)((uintptr_t)fdth + offset);
}

fdt_prop_t* fdt_get_prop_from_offset(fdt_header_t *fdth, unsigned int offset) {
    return (fdt_prop_t*)((uintptr_t)fdth + offset);
}

fdt_reserve_entry_t* fdt_get_rsv_from_offset(fdt_header_t *fdth, unsigned int offset) {
    return (fdt_reserve_entry_t*)((uintptr_t)fdth + offset);
}

const char* fdt_get_name_from_prop(fdt_header_t *fdth, fdt_prop_t *prop) {
    return (const char*)((uintptr_t)fdth + arch_rev32(fdth->off_dt_strings) + arch_rev32(prop->nameoff));
}

uint32_t fdt_get_len_from_prop(fdt_prop_t *prop) {
    return arch_rev32(prop->len);
}

uint32_t fdt_next_data_from_prop(fdt_prop_t *prop, unsigned int *data_offset) {
    if (arch_rev32(prop->token) != FDT_PROP || fdt_get_len_from_prop(prop) == 0) {
        *data_offset = 0;
        return 0;
    }

    uint32_t data = arch_rev32(*((uint32_t*)((uintptr_t)prop->data + *data_offset)));
    *data_offset += 4;
    *data_offset = *data_offset < fdt_get_len_from_prop(prop) ? *data_offset : 0;
    return data;
}

uint64_t fdt_next_data_cells_from_prop(fdt_prop_t *prop, unsigned int *data_offset, unsigned int num_cells) {
    uint64_t data = 0;
    for (unsigned int i = 0; i < num_cells; i++) {
        data = (data << 32) | fdt_next_data_from_prop(prop, data_offset);
    }
    return data;
}

const char* fdt_next_string_from_prop(fdt_prop_t *prop, unsigned int *string_offset) {
    if (arch_rev32(prop->token) != FDT_PROP || fdt_get_len_from_prop(prop) == 0) {
        *string_offset = 0;
        return 0;
    }

    const char *string = (const char*)((uintptr_t)prop->data + *string_offset);
    *string_offset += strlen(string) + 1;
    *string_offset = *string_offset < fdt_get_len_from_prop(prop) ? *string_offset : 0;
    return string;
}

unsigned int fdt_get_root_node(fdt_header_t *fdth) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;
    return arch_rev32(fdth->off_dt_struct);
}

unsigned int fdt_get_first_node(fdt_header_t *fdth) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;
    return fdt_next_subnode(fdth, arch_rev32(fdth->off_dt_struct));
}

unsigned int fdt_next_node(fdt_header_t *fdth, unsigned int offset) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;

    int subnode_depth = 0;
    fdt_token_t token;

    for (token = _fdt_next_token(fdth, &offset); token != FDT_END_NODE || subnode_depth > 0;
        token = _fdt_next_token(fdth, &offset)) {
        if (token == FDT_END || token == FDT_INVALID) return 0;

        // Keep track of subnodes while skipping over them
        if (token == FDT_BEGIN_NODE) subnode_depth++;
        else if (token == FDT_END_NODE) subnode_depth--;
    }

    // We've found the matching end node token for the previous node. Find the next begin node token
    for (token = _fdt_next_token(fdth, &offset); token != FDT_BEGIN_NODE; token = _fdt_next_token(fdth, &offset)) {
        if (token == FDT_END || token == FDT_INVALID) return 0;
    }

    return offset;
}

unsigned int fdt_next_subnode(fdt_header_t *fdth, unsigned int offset) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;

    for (fdt_token_t token = _fdt_next_token(fdth, &offset); token != FDT_BEGIN_NODE;
        token = _fdt_next_token(fdth, &offset)) {
        if (token != FDT_PROP && token != FDT_NOP) return 0;
    }

    return offset;
}

unsigned int fdt_next_prop(fdt_header_t *fdth, unsigned int offset) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;

    for (fdt_token_t token = _fdt_next_token(fdth, &offset); token != FDT_PROP;
        token = _fdt_next_token(fdth, &offset)) {
        if (token != FDT_NOP) return 0;
    }

    return offset;
}

unsigned int fdt_get_node(fdt_header_t *fdth, const char *path) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC || path[0] != '/') return 0;

    unsigned int offset = arch_rev32(fdth->off_dt_struct);
    const char *subpath = path;
    size_t num;

    do {
        // Split the path by '/', As we find nodes we will walk through the path string and go through the subnodes
        path = subpath;
        subpath = strchr(++path, '/');
        num = subpath == NULL ? strlen(path) : subpath - path;

        // Given the subnode offset, find a node with the name within it
        offset = fdt_next_subnode(fdth, offset);
        offset = _fdt_find_node_from_offset(fdth, offset, path, num);

        // If any point during the search we can't find a node, then return 0
        if (offset == 0) return 0;
    } while (subpath != NULL);

    return offset;
}

unsigned int fdt_get_node_from_phandle(fdt_header_t *fdth, int phandle) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;

    unsigned int offset = arch_rev32(fdth->off_dt_struct);

    do {
        // Check the phandle property
        unsigned int prop = fdt_get_prop(fdth, offset, "phandle");
        if (prop != 0) {
            unsigned int data_offset = 0;
            int ph = fdt_next_data_from_prop(fdt_get_prop_from_offset(fdth, prop), &data_offset);

            if (ph == phandle) return offset;
        }

        // Get the next sub node. If there is no next subnode then get the next node
        unsigned int next_offset = fdt_next_subnode(fdth, offset);
        if (next_offset == 0) {
            offset = fdt_next_node(fdth, offset);
        } else {
            offset = next_offset;
        }
    } while (offset != 0);

    return offset;
}

unsigned int fdt_get_prop(fdt_header_t *fdth, unsigned int node_offset, const char *name) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) return 0;

    fdt_node_t *node = fdt_get_node_from_offset(fdth, node_offset);
    if (arch_rev32(node->token) != FDT_BEGIN_NODE) return 0;

    unsigned int offset = node_offset;
    fdt_prop_t *prop;

    do {
        offset = fdt_next_prop(fdth, offset);
        prop = fdt_get_prop_from_offset(fdth, offset);
        if (strcmp(name, fdt_get_name_from_prop(fdth, prop)) == 0) break;
    } while (offset != 0);

    return offset;
}

void fdt_dump_header(fdt_header_t *fdth) {
    kprintf("\n");
    kprintf("FDT @ %#p\n", fdth);
    kprintf("totalsize: %#lx\n", arch_rev32(fdth->totalsize));
    kprintf("off_dt_struct: %#lx\n", arch_rev32(fdth->off_dt_struct));
    kprintf("off_dt_strings: %#lx\n", arch_rev32(fdth->off_dt_strings));
    kprintf("off_mem_rsvmap: %#lx\n", arch_rev32(fdth->off_mem_rsvmap));
    kprintf("version: %#lx\n", arch_rev32(fdth->version));
    kprintf("last_comp_version: %#lx\n", arch_rev32(fdth->last_comp_version));
    kprintf("boot_cpuid_phys: %#lx\n", arch_rev32(fdth->boot_cpuid_phys));
    kprintf("size_dt_string: %#lx\n", arch_rev32(fdth->size_dt_strings));
    kprintf("size_dt_struct: %#lx\n", arch_rev32(fdth->size_dt_struct));
    kprintf("\n");
}

void fdt_dump(fdt_header_t *fdth) {
    if (arch_rev32(fdth->magic) != FDT_MAGIC) {
        kprintf("No FDT found!\n");
        return;
    }

    fdt_dump_header(fdth);

    // Print the reserve entries
    for (fdt_reserve_entry_t *rsvmap = fdt_get_rsv_from_offset(fdth, arch_rev32(fdth->off_mem_rsvmap));
        arch_rev32(rsvmap->address) != 0 || arch_rev32(rsvmap->size) != 0; rsvmap++) {
        kprintf("Reserve: address = %#p, size = %#lx\n", arch_rev32(rsvmap->address), arch_rev32(rsvmap->size));
    }
    kprintf("\n");

    char tabs[16] = {0};
    unsigned int tab_level = 0, offset = arch_rev32(fdth->off_dt_struct);
    fdt_token_t token = _fdt_get_token_from_offset(fdth, offset);

    do {
        switch (token) {
            case FDT_BEGIN_NODE:
            {
                fdt_node_t *node = fdt_get_node_from_offset(fdth, offset);
                kprintf("%s%s {\n", tabs, node->name);
                tabs[tab_level++] = '\t';
                break;
            }
            case FDT_END_NODE:
            {
                tabs[--tab_level] = '\0';
                kprintf("%s}\n", tabs);
                break;
            }
            case FDT_PROP:
            {
                fdt_prop_t *prop = fdt_get_prop_from_offset(fdth, offset);
                const char *prop_name = fdt_get_name_from_prop(fdth, prop);
                uint32_t prop_len = fdt_get_len_from_prop(prop);

                kprintf("%s%s", tabs, prop_name);
                if (fdt_prop_data_is_string(prop)) {
                    kprintf(" = ");
                    for (unsigned int i = 0; i < prop_len; i += strlen((const char*)prop->data + i) + 1) {
                        kprintf("\"%s\" ", prop->data + i);
                    }
                } else if (prop_len != 0) {
                    kprintf(" = < ");
                    for (unsigned int i = 0; i < prop_len; i += 4) {
                        kprintf("%08x ", arch_rev32(*(uint32_t*)(prop->data + i)));
                    }
                    kprintf(">");
                }

                kprintf("\n");
                break;
            }
            case FDT_NOP: break;
            case FDT_END: break;
            default: kprintf("Unknown token: %#x @ %#p\n", token, (uintptr_t)fdth + offset); return;
        }

        token = _fdt_next_token(fdth, &offset);
    } while (token != FDT_END);
}
