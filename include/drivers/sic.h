#ifndef __SIC_H__
#define __SIC_H__

#define SIC_SOFTINT		(1 << 0)	// Software interrupt from SIC
#define SIC_MMCI0B		(1 << 1)	// Multimedia card 0B
#define SIC_MMCI1B		(1 << 2)	// Multimedia card 1B
#define SIC_KMI0		(1 << 3)	// Keyboard port
#define SIC_KMI1		(1 << 4)	// Mouse port
#define SIC_SCI1		(1 << 5)	// Smart card 1 interface
#define SIC_UART3		(1 << 6)	// UART 3
#define SIC_CHARLCD		(1 << 7)	// Character LCD
#define SIC_TOUCHSCRN	(1 << 8)	// Pen down on CLCD
#define SIC_KEYPAD		(1 << 9)	// Key pressed on digital keypad
#define SIC_DISKONCHIP	(1 << 21)	// DiskOnChip flash memory controller
#define SIC_MMCI0A		(1 << 22)	// Multimedia card 0A
#define SIC_MMCI1A		(1 << 23)	// Multimedia card 1A
#define SIC_AACI		(1 << 24)	// Audio CODEC interface
#define SIC_ETHERNET	(1 << 25)	// Ethernet controller
#define SIC_USB			(1 << 26)	// USB controller 
#define SIC_PCI0		(1 << 27)	// External PCI bus interrupt 0
#define SIC_PCI1		(1 << 28)	// External PCI bus interrupt 1
#define SIC_PCI2		(1 << 29)	// External PCI bus interrupt 2
#define SIC_PCI3		(1 << 30)	// External PCI bus interrupt 3

void sic_init();
uint32_t sic_status();
void sic_enable_all();
void sic_enable(uint32_t int_mask);
void sic_enable_soft_int(uint32_t soft_int_mask);
void sic_disable_all();
void sic_disable(uint32_t int_mask);
void sic_disable_soft_int(uint32_t soft_int_mask);

#endif // __SIC_H__

