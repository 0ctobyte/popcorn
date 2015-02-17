#include <stdint.h>

#include <platform/regs.h>

int32_t putchar(char c) {
	if(REG_RD32(R_UART0_FR) & (1 << 3)) return(-1);
	REG_WR32(R_UART0_DR, c);
	return(0);
}

