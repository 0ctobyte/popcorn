#ifndef __VIC_H__
#define __VIC_H__

#include <stdint.h>

// Initializes the Vectored Interrupt Controller
void vic_init();

// Enables a IRQ source on the VIC
void vic_enable(uint32_t bit);

// Disables a IRQ source on the VIC
void vic_disable(uint32_t bit);

// Gets the contents of the status register
uint32_t vic_status();

#endif // __VIC_H__

