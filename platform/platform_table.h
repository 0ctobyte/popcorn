#ifndef _PLATFORM_TABLE_H_
#define _PLATFORM_TABLE_H_

#include <kernel/fdt.h>

typedef struct {
    const char *compatible;
    void (*platform_init)(fdt_header_t *fdth);
} platform_table_entry_t;

extern void qemu_virt_platform_init(fdt_header_t *fdth);
extern void raspberry_pi_3_platform_init(fdt_header_t *fdth);
extern void raspberry_pi_4_platform_init(fdt_header_t *fdth);

#define PLATFORM_TABLE_SIZE (3)
#define PLATFORM_TABLE \
{\
    {"linux,dummy-virt", qemu_virt_platform_init},\
    {"raspberrypi,3-model-b", raspberry_pi_3_platform_init},\
    {"raspberrypi,4-model-b", raspberry_pi_4_platform_init},\
}

#endif // _PLATFORM_TABLE_H_
