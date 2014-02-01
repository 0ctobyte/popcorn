#include <stdbool.h>

#include <kernel/spinlock.h>
#include <kernel/atomic.h>
#include <kernel/kassert.h>

#include <mach/interrupts.h>

struct spinlock {
	uint32_t lock; // The lock value
	bool enabled;  // Saves the interrupt status state
};

struct spinlock_rw {
	uint32_t lock;
	uint32_t lightswitch;
};

void spin_init(spinlock_t *spin) {
	spin->lock = 0;
}

void spin_lock(spinlock_t *spin) {
	// Continuously attempt to set the value
	while(atomic_test_and_set(&(spin->lock)));
}

void spin_unlock(spinlock_t *spin) {
	// The spinlock must have been set previously
	kassert(spin->lock);
	spin->lock = 0;
}

void spin_irq_lock(spinlock_t *spin) {
	bool _enabled = interrupts_enabled();
	interrupts_disable();
	spin_lock(spin);
	spin->enabled = _enabled;
}

void spin_irq_unlock(spinlock_t *spin) {
	spin_unlock(spin);
	if(spin->enabled) interrupts_enable();
}

