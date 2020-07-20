#include <string.h>
#include <kernel/panic.h>
#include <kernel/kstdio.h>
#include <platform/platform_table.h>
#include <platform/platform.h>

void platform_init(fdt_header_t *fdth) {
    const platform_table_entry_t platform_table[PLATFORM_TABLE_SIZE] = PLATFORM_TABLE;
    const platform_table_entry_t *platform_entry = NULL;

    // Make sure we're running on the right platform
    unsigned int root = fdt_get_root_node(fdth);
    unsigned int prop = fdt_get_prop(fdth, root, "compatible");
    unsigned int offset = 0;
    const char *compatible = fdt_next_string_from_prop(fdt_get_prop_from_offset(fdth, prop), &offset);

    for (unsigned int i = 0; i < PLATFORM_TABLE_SIZE; i++) {
        if (strcmp(platform_table[i].compatible, compatible) == 0) {
            platform_entry = &platform_table[i];
            break;
        }
    }

    if (platform_entry == NULL) panic("platform_init: Unsupported platform!");

    platform_entry->platform_init(fdth);

    kprintf("Detected platform: %s\n", platform_entry->compatible);
}
