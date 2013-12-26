#ifndef __VIC_PL190_H__
#define __VIC_PL190_H__

#define VIC_WATCHDOG	(1 << 0)	// Watchdog timer
#define VIC_SOFTINT		(1 << 1)	// Software interrupt
#define VIC_COMMSRX		(1 << 2)	// Debug comms RX
#define VIC_COMMSTX		(1 << 3)	// Debug comms TX
#define VIC_TIMER01		(1 << 4)	// Timer 0 or 1
#define VIC_TIMER23		(1 << 5)	// Timer 2 or 3
#define VIC_GPIO0		(1 << 6)	// GPIO controller, pin 0
#define VIC_GPIO1		(1 << 7)	// GPIO controller, pin 1
#define VIC_GPIO2		(1 << 8)	// GPIO controller, pin 2
#define VIC_GPIO3		(1 << 9)	// GPIO controller, pin 3
#define VIC_RTC			(1 << 10)	// Real time clock
#define VIC_SSP			(1 << 11)	// Synchronous serial port
#define VIC_UART0		(1 << 12)	// UART 0
#define VIC_UART1		(1 << 13)	// UART 1
#define VIC_UART2		(1 << 14)	// UART 2
#define VIC_SCI0		(1 << 15)	// Smart card interface
#define VIC_CLCD		(1 << 16)	// Colour LCD controller
#define VIC_DMA			(1 << 17)	// Direct memory access
#define VIC_PWRFAIL		(1 << 18)	// FPGA power failure
#define VIC_MBX			(1 << 19)	// Graphics processor
#define VIC_GND			(1 << 20)	// Reserved
#define VIC_SRC21		(1 << 21)	// RealView Logic Tile or DiskOnChip
#define VIC_SRC22		(1 << 22)	// RealView Logic Tile or MCI0A		
#define VIC_SRC23		(1 << 23)	// RealView Logic Tile or MCI1A
#define VIC_SRC24		(1 << 24)	// RealView Logic Tile or AACI
#define VIC_SRC25		(1 << 25)	// RealView Logic Tile or Ethernet
#define VIC_SRC26		(1 << 26)	// RealView Logic Tile or USB
#define VIC_SRC27		(1 << 27)	// RealView Logic Tile or PCI0
#define VIC_SRC28		(1 << 28)	// RealView Logic Tile or PCI1
#define VIC_SRC29		(1 << 29)	// RealView Logic Tile or PCI2
#define VIC_SRC30		(1 << 30)	// RealView Logic Tile or PCI3
#define VIC_SIC			(1 << 31)	// Secondary interrupt controller

void vic_init();
uint32_t vic_status_irq();
void vic_enable_irq_all();
void vic_enable_irq(uint32_t irq_mask);
uint32_t vic_status_fiq();
void vic_enable_fiq(uint32_t fiq_mask);
void vic_enable_soft_int(uint32_t soft_int_mask);
void vic_disable_irq_all();
void vic_disable_irq(uint32_t irq_mask);
void vic_disable_fiq(uint32_t fiq_mask);
void vic_disable_soft_int(uint32_t soft_int_mask);

#endif // __VIC_PL190_H__

