#ifndef __HWREGS_H__
#define __HWREGS_H__

// UART0
#define R_UART0_PBASE 0x101F1000
#define R_UART0_VBASE 0xFFFF1000
#define O_UART0_DR 0
#define R_UART0_DR ((UART0_VBASE)+(O_UART0_DR))
#define O_UART0_FR 6
#define R_UART0_FR ((UART0_VBASE)+(O_UART0_FR))

#endif // __HWREGS_H__

