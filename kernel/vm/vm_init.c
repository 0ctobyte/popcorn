#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_map.h>
#include <kernel/vm/vm_km.h>
#include <kernel/vm/vm_init.h>

void vm_init(void) {
    pmap_bootstrap();

    pmap_init();
    vm_map_init();
    vm_object_init();
    vm_page_init();
    vm_km_init();
}
