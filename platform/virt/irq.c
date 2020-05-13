#include <platform/irq.h>
#include <platform/iomem.h>

static isr_t isr_table[64];

void irq_init() {
}

void irq_enable(irq_type_t type) {
    type++;
    return;
}

void irq_disable(irq_type_t type) {
    type++;
    return;
}

irq_type_t irq_get() {
    return(IRQ_NONE);
}

isr_t irq_get_isr(irq_type_t type) {
    return(isr_table[type]);
}

void irq_register_isr(irq_type_t type, isr_t func) {
    isr_table[type] = func;
}
