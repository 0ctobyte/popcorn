#include <stdint.h>

#include <platform/iomem.h>

int putchar(char c) {
    if(REG_RD32(R_UART0_SSR) & 1) return(-1);
    REG_WR32(R_UART0_THR, c);
    return(0);
}
