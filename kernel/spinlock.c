#include <kernel/spinlock.h>
#include <kernel/atomic.h>
#include <kernel/barrier.h>
#include <kernel/kassert.h>

#include <mach/interrupts.h>

// Meaning of the bits in the spinlock's lock variable
// Bit 0 is the lock value: 0 for unlocked, 1 for locked
#define SPIN_LOCK (0x1)
// Bit 1 is the status of interrupts: 1 for enabled, 0 for disabled
#define SPIN_ENABLED (0x2)
// Bit 2 is for the R/W value when using readlock and writelock
#define SPIN_LIGHTSWITCH (0x4)

// The counter for the number of readlocks is stored starting from bit 3 in the
// lock variable
#define GET_READLOCK_COUNT(B) (((B)->lock) >> 3)
#define SET_READLOCK_COUNT(B, val) (((B)->lock) |= ((val) << 3))

// Set enabled bit, get enabled bit
#define GET_ENABLED_BIT(B) (((B)->lock) & SPIN_ENABLED)
#define SET_ENABLED_BIT(B, val) (((B)->lock) |= (val << 1))

// Attempts to gain lock using the specified bit(s)
void _spin_lock(spinlock_t *spin, uint32_t bits) {
	// Continuously attempt to set the bit, the loop will exit only when we 
	// succeed in setting the bit
	while(atomic_test_and_set_bit(&(spin->lock), (bits)));
	// Membar after spinlock has been acquired
	barrier_dmb();
}

// Unlocks using the specified bit(s)
void _spin_unlock(spinlock_t *spin, uint32_t bits) {
	// Make sure all accesses to resource protected by this spinlock have
	// completed
	barrier_dmb();
	// Clear bits
	spin->lock &= ~(bits);
	// Ensure all memory accesses to spinlock have completed before proceeding
	// with execution of code
	barrier_dsb();
}

void spin_init(spinlock_t *spin) {
	spin->lock = 0;
}

void spin_lock(spinlock_t *spin) {
	_spin_lock(spin, SPIN_LOCK);
}

void spin_unlock(spinlock_t *spin) {
	// The spinlock must have been set previously
	kassert(spin->lock & SPIN_LOCK);
	_spin_unlock(spin, SPIN_LOCK | SPIN_ENABLED);
}

bool spin_trylock(spinlock_t *spin) {
	return(!atomic_test_and_set_bit(&(spin->lock), SPIN_LOCK));
}

void spin_irqlock(spinlock_t *spin) {
	// Get state of interrupts
	bool _enabled = interrupts_enabled();
	interrupts_disable();
	spin_lock(spin);

	// Set enabled bit to 1 if interrupts were enabled
	SET_ENABLED_BIT(spin, _enabled);
	barrier_dmb();
}

void spin_irqunlock(spinlock_t *spin) {
	// Get the previous interrupt enabled state
	bool _enabled = GET_ENABLED_BIT(spin);
	spin_unlock(spin);
	if(_enabled) interrupts_enable();

	// Ensure previous instructions complete 
	barrier_dsb();
}

void spin_readlock(spinlock_t *spin) {
	// Attempt to set the lightswitch bit
	_spin_lock(spin, SPIN_LIGHTSWITCH);

	// Once acquired, increment the count
	uint32_t count = GET_READLOCK_COUNT(spin);
	SET_READLOCK_COUNT(spin, ++count);

	// The first reader must wait for lock, subsequent readers will get access
	// immediately
	if(count == 1) spin_lock(spin);

	// Once acquired release lightswitch lock
	_spin_unlock(spin, SPIN_LIGHTSWITCH);
}

void spin_readunlock(spinlock_t *spin) {
	// Attempt to set the lightswitch bit
	_spin_unlock(spin, SPIN_LIGHTSWITCH);

	// Once acquired, decrement the count
	uint32_t count = GET_READLOCK_COUNT(spin);
	SET_READLOCK_COUNT(spin, --count);

	// The last reader must release the spinlock
	if(count == 0) spin_unlock(spin);

	// Release the lightswitch lock
	_spin_unlock(spin, SPIN_LIGHTSWITCH);
}

void spin_writelock(spinlock_t *spin) {
	// Writers must spin until all readers have finished using resource
	// then block all readers/writers from using resource
	// This can be done by simply locking the spinlock
	spin_lock(spin);
}

void spin_writeunlock(spinlock_t *spin) {
	// Release the spinlock
	spin_unlock(spin);
}

