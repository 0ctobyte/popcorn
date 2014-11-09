#ifndef __SIC_H__
#define __SIC_H__

#include <stdint.h>

// Initializes the Secondary Interrupt Controller
void sic_init();

// Enables a IRQ source on the SIC
void sic_enable(uint32_t bit);

// Disables a IRQ source on the SIC
void sic_disable(uint32_t bit);

// Gets the contents of the status register
uint32_t sic_status();

#endif // __SIC_H__

