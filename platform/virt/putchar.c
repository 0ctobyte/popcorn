#include <stdint.h>

#include <platform/iomem.h>

long putchar(char c) {
    // Check if transmit FIFO is full
    if(REG_RD32(R_UART0_FR) & (1 << 5)) return(-1);
    REG_WR32(R_UART0_DR, c);
    return(0);
}
