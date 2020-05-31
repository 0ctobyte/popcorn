#include <kernel/vm_map.h>
#include <kernel/vm_page.h>
#include <kernel/kassert.h>

#include <string.h>

// Kernel vmap
vm_map_t kernel_vmap;

void vm_map_bootstrap(void) {
}
