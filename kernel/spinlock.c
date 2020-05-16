#include <kernel/spinlock.h>
#include <kernel/atomic.h>
#include <kernel/barrier.h>
#include <kernel/kassert.h>
#include <kernel/interrupts.h>

// Meaning of the bits in the spinlock's lock variable
// Bit 0 is the lock value: 0 for unlocked, 1 for acquired
#define SPINLOCK_ACQUIRED (0x1)
// Bit 1 is the status of interrupts: 1 for enabled, 0 for disabled
#define SPINLOCK_ENABLED (0x2)
// Bit 2 is for the R/W value when using readacquire and writeacquire
#define SPINLOCK_LIGHTSWITCH (0x4)

// The counter for the number of readacquires is stored starting from bit 3 in the lock variable
#define GET_READACQUIRE_COUNT(B) ((*B) >> 3)
#define SET_READACQUIRE_COUNT(B, val) ((*B) |= ((val) << 3))

// Set enabled bit, get enabled bit
#define GET_ENABLED_BIT(B) ((*B) & SPINLOCK_ENABLED)
#define SET_ENABLED_BIT(B, val) ((*B) |= (val << 1))

/*
 * Spinlocks are basically busywait mutual exclusion locks. When a lock cannot
 * be acquired, the thread will continuously attempt to gain the lock.
 * Spinlocks should only be used if the lock will only be acquired for a small
 * amount of time.
 *
 * This spinlock implementation allows for readers/writers locking. This means
 * that threads that only intend on reading from a resource in memory may do so
 * simultaneously while threads that intend on writing to the resource may only
 * do so one at a time blocking all other threads from accessing the resource.
 * Note: this implementation gives priority to reader threads which may
 * cause writer threads to starve.
 *
 * The spinlock uses only one integer where certain bits are used for
 * certain purposes. Bit 0 is used as the main lock. Bit 1 is used to hold
 * the state of interrupts, if 1 then interrupts were previously enabled
 * before the call to irqlock. Bit 2 is used as the "lightswitch" lock.
 * This is only used when using readacquire/readrelease functions. It basically
 * allows only one thread to modify the "count" (count of how many readers
 * have acquired a lock) which starts from bit 3 in the lock variable.
 */

// Attempts to gain lock using the specified bit(s)
void _spinlock_acquire(spinlock_t *lock, unsigned long bits) {
    // Continuously attempt to set the bit, the loop will exit only when we
    // succeed in setting the bit
    while(atomic_test_and_set_bit(lock, (bits)));
    // Membar after spinlock has been acquired
    barrier_dmb();
}

// Unlocks using the specified bit(s)
void _spinlock_release(spinlock_t *lock, unsigned long bits) {
    // Make sure all accesses to resource protected by this spinlock have
    // completed
    barrier_dmb();
    // Clear bits
    *lock &= ~(bits);
    // Ensure all memory accesses to spinlock have completed before proceeding
    // with execution of code
    barrier_dsb();
}

void spinlock_init(spinlock_t *lock) {
    lock = 0;
}

void spinlock_acquire(spinlock_t *lock) {
    _spinlock_acquire(lock, SPINLOCK_ACQUIRED);
}

void spinlock_release(spinlock_t *lock) {
    // The spinlock must have been set previously
    kassert(*lock & SPINLOCK_ACQUIRED);
    _spinlock_release(lock, SPINLOCK_ACQUIRED | SPINLOCK_ENABLED);
}

bool spinlock_tryacquire(spinlock_t *lock) {
    return(!atomic_test_and_set_bit(lock, SPINLOCK_ACQUIRED));
}

void spinlock_irqacquire(spinlock_t *lock) {
    // Get state of interrupts
    bool _enabled = interrupts_enabled();
    interrupts_disable();
    spinlock_acquire(lock);

    // Set enabled bit to 1 if interrupts were enabled
    SET_ENABLED_BIT(lock, _enabled);
    barrier_dmb();
}

void spinlock_irqrelease(spinlock_t *lock) {
    // Get the previous interrupt enabled state
    bool _enabled = GET_ENABLED_BIT(lock);
    spinlock_release(lock);
    if(_enabled) interrupts_enable();

    // Ensure previous instructions complete
    barrier_dsb();
}

void spinlock_readacquire(spinlock_t *lock) {
    // Attempt to set the lightswitch bit
    _spinlock_acquire(lock, SPINLOCK_LIGHTSWITCH);

    // Once acquired, increment the count
    unsigned long count = GET_READACQUIRE_COUNT(lock);
    SET_READACQUIRE_COUNT(lock, ++count);

    // The first reader must wait for lock, subsequent readers will get access
    // immediately
    if(count == 1) spinlock_acquire(lock);

    // Once acquired release lightswitch lock
    _spinlock_release(lock, SPINLOCK_LIGHTSWITCH);
}

void spinlock_readrelease(spinlock_t *lock) {
    // Attempt to set the lightswitch bit
    _spinlock_release(lock, SPINLOCK_LIGHTSWITCH);

    // Once acquired, decrement the count
    unsigned long count = GET_READACQUIRE_COUNT(lock);
    SET_READACQUIRE_COUNT(lock, --count);

    // The last reader must release the spinlock
    if(count == 0) spinlock_release(lock);

    // Release the lightswitch lock
    _spinlock_release(lock, SPINLOCK_LIGHTSWITCH);
}

void spinlock_writeacquire(spinlock_t *lock) {
    // Writers must spin until all readers have finished using resource then block all readers/writers from using resource
    // This can be done by simply locking the spinlock
    spinlock_acquire(lock);
}

void spinlock_writerelease(spinlock_t *lock) {
    // Release the spinlock
    spinlock_release(lock);
}
