#include <stdint.h>

#include <platform/iomem.h>

int32_t putchar(char c) {
	if(REG_RD32(R_UART0_SSR) & 1) return(-1);
	REG_WR32(R_UART0_THR, c);
  // Send a carriage return after a newline
  if(c == '\n') while(putchar('\r') == -1) continue;
	return(0);
}

