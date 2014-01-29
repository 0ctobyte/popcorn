#include <stdint.h>

#define UART0DR (0x0)
#define UART0FR (0x18)

volatile uint32_t *UART0BASEADDRESS = (uint32_t*)0xFFFF1000;

uint32_t putchar(char c) {
	if(*(UART0BASEADDRESS+UART0FR) & (1 << 3)) return(-1);
	*(UART0BASEADDRESS+UART0DR) = c;
	return(0);
}

