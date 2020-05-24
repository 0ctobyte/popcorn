#include <stdint.h>

// UART0
#define R_UART0_BASE (0x09000000)
#define O_UART0_DR (0x0)
#define O_UART0_FR (0x18)
#define O_UART0_IMSC (0x038)

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

uintptr_t uart_base_addr = R_UART0_BASE;

long putchar(char c) {
    // Check if transmit FIFO is full
    if(REG_RD32(uart_base_addr + O_UART0_FR) & (1 << 5)) return(-1);
    REG_WR32(uart_base_addr + O_UART0_DR, c);
    return(0);
}
