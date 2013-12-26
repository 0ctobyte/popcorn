#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#define INTERRUPT_LOCK bool reenable_interrupts = interrupts_enabled(); \
												  disable_interrupts();
#define INTERRUPT_UNLOCK if(reenable_interrupts) enable_interrupts();

typedef enum {
	IRQ_WATCHDOG	= 0,
	IRQ_VICSOFTINT	= 1,
	IRQ_COMMSRX		= 2,
	IRQ_COMMSTX		= 3,
	IRQ_TIMER01		= 4,
	IRQ_TIMER23		= 5,
	IRQ_GPIO0		= 6,
	IRQ_GPIO1		= 7,
	IRQ_GPIO2		= 8,
	IRQ_GPIO3		= 9,
	IRQ_RTC			= 10,
	IRQ_SSP			= 11,
	IRQ_UART0		= 12,
	IRQ_UART1		= 13,
	IRQ_UART2		= 14,
	IRQ_SCI0		= 15,
	IRQ_CLCD		= 16,
	IRQ_DMA			= 17,
	IRQ_PWRFAIL		= 18,
	IRQ_MBX			= 19,
	IRQ_GND			= 20,
	IRQ_SRC21		= 21,
	IRQ_SRC22		= 22,
	IRQ_SRC23		= 23,
	IRQ_SRC24		= 24,
	IRQ_SRC25		= 25,
	IRQ_SRC26		= 26,
	IRQ_SRC27		= 27,
	IRQ_SRC28		= 28,
	IRQ_SRC29		= 29,
	IRQ_SRC30		= 30,
	IRQ_SIC			= 31,
	IRQ_SICSOFTINT	= 32,
	IRQ_MMCI0B		= 33,
	IRQ_MMCI1B		= 34,
	IRQ_KMI0		= 35,
	IRQ_KMI1		= 36,
	IRQ_SCI1		= 37,
	IRQ_UART3		= 38,
	IRQ_CHARLCD		= 39,
	IRQ_TOUCHSCRN	= 40,
	IRQ_KEYPAD		= 41,
	IRQ_DISKONCHIP	= 53,
	IRQ_MMCI0A		= 54,
	IRQ_MMCI1A		= 55,
	IRQ_AACI		= 56,
	IRQ_ETHERNET	= 57,
	IRQ_USB			= 58,
	IRQ_PCI0		= 59,
	IRQ_PCI1		= 60,
	IRQ_PCI2		= 61,
	IRQ_PCI3		= 62
} irq_type_t;

typedef uint32_t (*isr_t)(uint32_t);

void enable_interrupts();
void disable_interrupts();
bool interrupts_enabled();

void enable_irq(irq_type_t);
void register_isr(irq_type_t, isr_t);

#endif // __INTERRUPTS_H__

