#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/serial/bcm2387-miniuart/bcm2387_miniuart.h>
#include <platform/platform.h>

//#define R_UART0_BASE (0x3F201000) // Raspberry Pi 3 UART0
#define MU_BASE (0x3f215040)
#define MU_SIZE (0x1000)

bcm2387_miniuart_t bcm2387_miniuart;

void platform_early_init(fdt_header_t *fdth) {
}

void platform_init(fdt_header_t *fdth) {
    bcm2387_miniuart.uart_base = max_kernel_virtual_end + MU_BASE;

    uintptr_t mu_base = MU_BASE & ~(PAGESIZE - 1);
    uintptr_t uart_base = bcm2387_miniuart.uart_base & ~(PAGESIZE - 1);
    for (size_t offset = 0; offset < MU_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(uart_base + offset, MU_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    console_dev.data = (void*)&bcm2387_miniuart;
    console_dev.ops = &bcm2387_miniuart_ops;
}
