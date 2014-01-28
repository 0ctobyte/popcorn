#include "vic.h"

// Register offsets
#define VICIRQSTATUS (0x0)
#define VICFIQSTATUS (0x4)
#define VICRAWINTR (0x8)
#define VICINTSELECT (0xC)
#define VICINTENABLE (0x10)
#define VICINTENCLEAR (0x14)
#define VICSOFTINT (0x18)
#define VICSOFTINTCLEAR (0x1C)
#define VICPROTECTION (0x20)

volatile uint32_t *VICBASEADDRESS = (uint32_t*)0xFFFF0000;

void vic_init() {
	*(VICBASEADDRESS+VICINTSELECT) = 0;
	*(VICBASEADDRESS+VICINTENABLE) = 0;
	*(VICBASEADDRESS+VICSOFTINT) = 0;
}

void vic_enable(uint32_t bit) {
	*(VICBASEADDRESS+VICINTENABLE) |= bit;
}

void vic_disable(uint32_t bit) {
	*(VICBASEADDRESS+VICINTENCLEAR) = bit;
}

uint32_t vic_status() {
	return(*(VICBASEADDRESS+VICIRQSTATUS));
}

