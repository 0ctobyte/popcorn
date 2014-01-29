#include "vic.h"

// Register offsets
#define VICIRQSTATUS (0)
#define VICFIQSTATUS (1)
#define VICRAWINTR (2)
#define VICINTSELECT (3)
#define VICINTENABLE (4)
#define VICINTENCLEAR (5)
#define VICSOFTINT (6)
#define VICSOFTINTCLEAR (7)
#define VICPROTECTION (8)

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

