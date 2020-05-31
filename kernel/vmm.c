#include <kernel/vmm.h>
#include <kernel/pmap.h>
#include <kernel/vm_map.h>

void vmm_init(void) {
    pmap_bootstrap();

    // FIXME switching UART to VA address
    extern unsigned long uart_base_addr;
    vaddr_t uart_base_va = 0xFFFFC00000000000;
    pmap_kenter_pa(uart_base_va, uart_base_addr & ~(PAGESIZE - 1), VM_PROT_DEFAULT, PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    uart_base_addr = uart_base_va + (uart_base_addr & (PAGESIZE - 1));

    vm_map_bootstrap();
}
