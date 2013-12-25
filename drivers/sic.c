// Driver for the secondary interrupt controller

#include <stdint.h>

#include "../include/drivers/sic.h"

#define SICBASEADDRESS 0x10003000

typedef volatile struct {
	// R - Indicates sources which have been masked in SICENABLE and have raised
	// an interrupt
	uint32_t SICSTATUS;

	// R - Indicates sources that have raised an interrupt regardless of their
	// masking status in SICENABLE
	uint32_t SICRAWSTAT;

	// R/W - Source generates interrupt if bit is set to HIGH
	// In the datasheets, there are two registers, SICENABLE and SICENSET.
	// SICENABLE is read only and SICENSET is write only, they are both mapped
	// to the same address. We will only use one variable to represent both
	// in this struct.
	uint32_t SICENABLE;

	// W - If bit is set to HIGH, corresponding bit in SICENABLE is cleared 
	uint32_t SICENCLR;

	// R/W - Set software interrupt for specified source(s)
	uint32_t SICSOFTINTSET;

	// W - If bit is set to HIGH, corresponding bit in SICSOFTINTSET is cleared
	uint32_t SICSOFTINTCLR;

	// Padding
	uint32_t SICUNUSED[2];

	// R/W - Set bits to HIGH allows source interrupt to pass through directly
	// to the primary interrupt controller (VIC). Only works for sources 21 to 30
	// There are two registers SICPICENABLE and SICPICENSET. SICPICENABLE is read
	// only whereas SICPICENSET is write only. They are both mapped to the same
	// address. We will only use one variable to represent both here.
	uint32_t SICPICENABLE;

	// W - Set biths to HIGH to clear corresponding bits in SICPICENABLE
	uint32_t SICPICENCLR;
} sic_t;

sic_t *sic = (sic_t*)SICBASEADDRESS;

void sic_init() {
	// Clear registers first
	sic->SICENABLE = 0;
	sic->SICSOFTINTSET = 0;
	sic->SICPICENABLE = 0;
}

uint32_t sic_status() {
	return(sic->SICSTATUS);
}

void sic_enable_all() {
	sic->SICENABLE = 0xFFFFFFFF;
}

void sic_enable(uint32_t int_mask) {
	sic->SICENABLE |= int_mask;
}

void sic_enable_soft_int(uint32_t soft_int_mask) {
	sic->SICSOFTINTSET |= soft_int_mask;
}

void sic_disable_all() {
	sic->SICENABLE = 0;
}

void sic_disable(uint32_t int_mask) {
	sic->SICENCLR = int_mask;
}

void sic_disable_soft_int(uint32_t soft_int_mask) {
	sic->SICSOFTINTCLR = soft_int_mask;
}

