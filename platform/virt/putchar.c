#include <stdint.h>

#include <platform/iomem.h>

uintptr_t uart_base_addr = R_UART0_BASE;

long putchar(char c) {
    // Check if transmit FIFO is full
    if(REG_RD32(uart_base_addr + O_UART0_FR) & (1 << 5)) return(-1);
    REG_WR32(uart_base_addr + O_UART0_DR, c);
    return(0);
}
