#include <stdint.h>

// UART0
#define R_MU_BASE (0x3f215040)
#define O_MU_IO   (0x00)
#define O_MU_LSR  (0x14)
#define O_MU_STAT (0x24)
#define O_MU_IIR  (0x08)

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

uintptr_t uart_base_addr = R_MU_BASE;

long putchar(char c) {
    // Check if transmit FIFO is full
    if(!(REG_RD32(uart_base_addr + O_MU_LSR) & (1 << 5))) return(-1);
    REG_WR32(uart_base_addr + O_MU_IO, c);
    return(0);
}
