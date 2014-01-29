#include "sic.h"

// Register offsets
#define SICSTATUS (0)
#define SICRAWSTAT (1)
#define SICENABLE (2)
#define SICENCLR (3)
#define SICSOFTINTSET (4)
#define SICSOFTINTCLR (5)
#define SICPICENABLE (8)
#define SICPICENCLR (9)

volatile uint32_t *SICBASEADDRESS = (uint32_t*)0xFFFF3000;

void sic_init() {
	*(SICBASEADDRESS+SICENABLE) = 0;
	*(SICBASEADDRESS+SICSOFTINTSET) = 0;
	*(SICBASEADDRESS+SICPICENABLE) = 0;
}

void sic_enable(uint32_t bit) {
	*(SICBASEADDRESS+SICENABLE) |= bit;
}

void sic_disable(uint32_t bit) {
	*(SICBASEADDRESS+SICENCLR) = bit;
}

uint32_t sic_status() {
	return(*(SICBASEADDRESS+SICSTATUS));
}

