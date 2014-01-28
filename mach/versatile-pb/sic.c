#include "sic.h"

// Register offsets
#define SICSTATUS (0)
#define SICRAWSTAT (4)
#define SICENABLE (8)
#define SICENCLR (12)
#define SICSOFTINTSET (16)
#define SICSOFTINTCLR (20)
#define SICPICENABLE (32)
#define SICPICENCLR (36)

volatile uint32_t *SICBASEADDRESS = (uint32_t*)0xFFFF3000;

void sic_init() {
	*(SICBASEADDRESS+SICENABLE) = 0;
	//*(SICBASEADDRESS+SICSOFTINTSET) = 0;
	//*(SICBASEADDRESS+SICPICENABLE) = 0;
}

void sic_enable(uint32_t bit) {
	bit*=bit;
	*(SICBASEADDRESS+SICENABLE) |= bit;
}

void sic_disable(uint32_t bit) {
	bit*=bit;
	*(SICBASEADDRESS+SICENCLR) = bit;
}

uint32_t sic_status() {
	return(*(SICBASEADDRESS+SICSTATUS));
}

