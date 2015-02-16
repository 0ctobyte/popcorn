#include <platform/interrupts.h>
#include <platform/regs.h>

isr_t isr_table[64];

void vic_init() {
  REG_WR32(R_VIC_INTSELECT, 0);
  REG_WR32(R_VIC_INTENABLE, 0);
  REG_WR32(R_VIC_SOFTINT, 0);
}

void vic_enable(uint32_t bit) {
  REG_WR32(R_VIC_INTENABLE, REG_RD32(R_VIC_INTENABLE) | bit);
}

void vic_disable(uint32_t bit) {
  REG_WR32(R_VIC_INTENCLEAR, bit);
}

uint32_t vic_status() {
	return(REG_RD32(R_VIC_IRQSTATUS));
}

void sic_init() {
  REG_WR32(R_SIC_ENABLE, 0);
  REG_WR32(R_SIC_SOFTINTSET, 0);
  REG_WR32(R_SIC_PICENABLE, 0);
}

void sic_enable(uint32_t bit) {
  REG_WR32(R_SIC_ENABLE, REG_RD32(R_SIC_ENABLE) | bit);
}

void sic_disable(uint32_t bit) {
  REG_WR32(R_SIC_ENCLR, bit);
}

uint32_t sic_status() {
	return(REG_RD32(R_SIC_STATUS));
}

void irq_init() {
	// Init the interrupt controllers
	vic_init();
	sic_init();

	// Enable IRQ's from the SIC
	irq_enable(IRQ_SIC);

	// Enable interrupts on the CPU
	interrupts_enable();
}

void irq_enable(irq_type_t type) {
	// If type is less than 32, than the irq is on the VIC, otherwise the IRQ
	// is on the SIC
	if(type < 32) vic_enable((1 << type));
	else sic_enable((1 << (type-32)));
}

void irq_disable(irq_type_t type) {
	// Similar logic to irq_enable
	if(type < 32) vic_disable((1 << type));
	else sic_disable((1 << (type-32)));
}

irq_type_t irq_get() {
	uint32_t status = vic_status();
	uint32_t count;
	for(count = 0; count < 32; count++) {
		if(status & (1 << count)) break;
	}

	if(count < 31) return(count);
	if(count > 31) return(IRQ_NONE);

	status = sic_status();
	++count;
	for(uint32_t i = 0; i < 32; i++, count++) {
		if(status & (1 << i)) break;
	}

	if(count >= 64) count = IRQ_NONE;
	
	return(count);
}

isr_t irq_get_isr(irq_type_t type) {
	return(isr_table[type]);
}

void irq_register_isr(irq_type_t type, isr_t func) {
	isr_table[type] = func;
}

