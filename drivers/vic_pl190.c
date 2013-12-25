// Driver for the VIC (Vectored Interrupt Controller) PL190
// This is the primary interrupt controller on the versatile pb board

#include <stdint.h>

#include "../include/drivers/vic_pl190.h"

#define VICBASEADDRESS 0x10140000

typedef volatile struct {
	// R - Indicates sources who have been masked in VICINTENABLE and have raised
	// an interrupt
	uint32_t VICIRQSTATUS;
	
	// R - Indicates sources who have been masked in VICINTENABLE and VICINTSELECT
	// and have raised an interrupt
	uint32_t VICFIQSTATUS;

	// R - Indicates sources that have raised an interrupt regardless of
	// their masking status in VICINTENABLE and VICINTSELECT
	uint32_t VICRAWINTR;

	// R/W - Source generates FIQ if bit is set to HIGH, IRQ is generated if LOW
	uint32_t VICINTSELECT;

	// R/W - Source generates IRQ/FIQ if bit is set to HIGH
	uint32_t VICINTENABLE;

	// W - If bit is set to HIGH, corresponding bit in VICINTENABLE is cleared
	// A bit set to LOW has no effect
	uint32_t VICINTENCLEAR;

	// R/W - Setting a bit to HIGH generates a software interrupt for the source
	// if corresponding bit in VICINTENABLE is set to HIGH
	uint32_t VICSOFTINT;

	// W - If bit is set to HIGH, corresponding bit in VICSOFTINT is cleared
	// A LOW bit has no effect
	uint32_t VICSOFTINTCLEAR;

	// R/W - Only the 0th bit is used; if set, VIC registers can only be accessed
	// in protected mode (no user mode access)
	uint32_t VICPROTECTION;

	// Padding
	uint32_t VICUNUSED1[3];

	// R/W - Contains the ISR address of the currently active interrupt
	// Only used if vectored interrupts are used
	uint32_t VICVECTADDR;

	// R/W - Contains the default ISR address
	uint32_t VICDEFVECTADDR;

	// There are more registers, but the above are the only important ones
} vic_pl190_t;

vic_pl190_t *vic = (vic_pl190_t*)VICBASEADDRESS;
uint32_t VICINTENABLE_saved = 0;

// Clears enable/select registers, sets the protection bit
void vic_init() {
	// Clear registers first
	vic->VICINTSELECT = 0;
	vic->VICINTENABLE = 0;
	vic->VICSOFTINT = 0;

	// Enable protection
	vic->VICPROTECTION = 1;
}

uint32_t vic_status_irq() {
	return(vic->VICIRQSTATUS);
}

void vic_enable_irq_all() {
	vic->VICINTENABLE = 0xFFFFFFFF;
}

void vic_enable_irq(uint32_t irq_mask) {
	vic->VICINTENABLE |= irq_mask;
}

uint32_t vic_status_fiq() {
	return(vic->VICFIQSTATUS);
}

void vic_enable_fiq(uint32_t fiq_mask) {
	vic->VICINTSELECT |= fiq_mask;
	vic->VICINTENABLE |= fiq_mask;
}

void vic_enable_soft_int(uint32_t soft_int_mask) {
	vic->VICSOFTINT |= soft_int_mask;
	vic->VICINTENABLE |= soft_int_mask;
}

void vic_disable_irq_all() {
	VICINTENABLE_saved = vic->VICINTENABLE;
	vic->VICINTENABLE = 0;
}

void vic_disable_irq(uint32_t irq_mask) {
	vic->VICINTENCLEAR = irq_mask;
}

void vic_disable_fiq(uint32_t fiq_mask) {
	vic->VICINTSELECT &= (~fiq_mask);
	vic->VICINTENCLEAR = fiq_mask;
}

void vic_disable_soft_int(uint32_t soft_int_mask) {
	vic->VICSOFTINTCLEAR = soft_int_mask;
	vic->VICINTENCLEAR = soft_int_mask;
}

