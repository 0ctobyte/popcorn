#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <devices/serial/bcm2385-aux-uart/bcm2385_aux_uart.h>
#include <platform/platform.h>

//#define R_UART0_BASE (0x3F201000) // Raspberry Pi 3 UART0
#define MU_BASE (0x3f215040)
#define MU_SIZE (0x1000)

bcm2385_aux_uart_t bcm2385_aux_uart;

void platform_early_init(fdt_header_t *fdth) {
}

void platform_init(fdt_header_t *fdth) {
    bcm2385_aux_uart.uart_base = max_kernel_virtual_end + MU_BASE;

    uintptr_t mu_base = MU_BASE & ~(PAGESIZE - 1);
    uintptr_t uart_base = bcm2385_aux_uart.uart_base & ~(PAGESIZE - 1);
    for (size_t offset = 0; offset < MU_SIZE; offset += PAGESIZE) {
        pmap_kenter_pa(uart_base + offset, MU_BASE + offset, VM_PROT_DEFAULT,
            PMAP_FLAGS_READ | PMAP_FLAGS_WRITE | PMAP_FLAGS_NOCACHE);
    }

    console_dev.data = (void*)&bcm2385_aux_uart;
    console_dev.ops = &bcm2385_aux_uart_ops;
}
