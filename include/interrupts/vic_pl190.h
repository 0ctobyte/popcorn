#ifndef __VIC_PL190__
#define __VIC_PL190__

void vic_init();
void vic_enable_irq_all();
void vic_enable_irq(uint32_t irq_mask);
void vic_enable_fiq(uint32_t fiq_mask);
void vic_enable_soft_int(uint32_t soft_int_mask);
void vic_disable_irq_all();
void vic_disable_irq(uint32_t irq_mask);
void vic_disable_fiq(uint32_t fiq_mask);
void vic_disable_soft_int(uint32_t soft_int_mask);

#endif // __VIC_PL190__

