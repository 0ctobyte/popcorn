/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/arch/arch_atomic.h>
#include <kernel/arch/arch_barrier.h>
#include <kernel/arch/arch_interrupts.h>
#include <kernel/spinlock.h>

// Meaning of the bits in the spinlock's lock variable
// Bit 0 is the lock value: 0 for unlocked, 1 for acquired
#define SPINLOCK_ACQUIRED              (0x1)
// Bit 1 is the status of interrupts: 1 for enabled, 0 for disabled
#define SPINLOCK_ENABLED               (0x2)
// Bit 2 is for the R/W value when using readacquire and writeacquire
#define SPINLOCK_LIGHTSWITCH           (0x4)

// The counter for the number of readers holding the lock is stored starting from bit 3 in the lock variable
#define GET_READ_ACQUIRE_COUNT(B)      (*(B) >> 3)
#define SET_READ_ACQUIRE_COUNT(B, val) (*(B) |= ((val) << 3))

// Set enabled bit, get enabled bit
#define GET_ENABLED_BIT(B)             (*(B) & SPINLOCK_ENABLED)
#define SET_ENABLED_BIT(B, val)        (*(B) |= ((val) << 1))

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
 * This is only used when using read_acquire/read_release functions. It basically
 * allows only one thread to modify the "count" (count of how many readers
 * have acquired a lock) which starts from bit 3 in the lock variable.
 */

void _spinlock_acquire(spinlock_t *lock, unsigned long bits) {
    // Continuously attempt to set the bit, the loop will exit only when we succeed in setting the bit
    while (arch_atomic_test_and_set_bit(lock, (bits)));
    arch_barrier_dmb();
}

void _spinlock_release(spinlock_t *lock, unsigned long bits) {
    // Make sure all accesses to resource protected by this spinlock have completed before clearing the bit
    arch_barrier_dmb();
    *lock &= ~(bits);
}

void spinlock_acquire(spinlock_t *lock) {
    _spinlock_acquire(lock, SPINLOCK_ACQUIRED);
}

void spinlock_release(spinlock_t *lock) {
    kassert(*lock & SPINLOCK_ACQUIRED);
    _spinlock_release(lock, SPINLOCK_ACQUIRED | SPINLOCK_ENABLED);
}

bool spinlock_try_acquire(spinlock_t *lock) {
    return !arch_atomic_test_and_set_bit(lock, SPINLOCK_ACQUIRED);
}

void spinlock_irq_acquire(spinlock_t *lock) {
    // Get state of interrupts
    bool _enabled = arch_interrupts_is_enabled();
    arch_interrupts_disable();
    spinlock_acquire(lock);

    // Set enabled bit to 1 if interrupts were enabled
    SET_ENABLED_BIT(lock, _enabled);
}

void spinlock_irq_release(spinlock_t *lock) {
    // Get the previous interrupt enabled state
    bool _enabled = GET_ENABLED_BIT(lock);
    spinlock_release(lock);
    if(_enabled) arch_interrupts_enable();
}

void spinlock_read_acquire(spinlock_t *lock) {
    // Attempt to set the lightswitch bit
    _spinlock_acquire(lock, SPINLOCK_LIGHTSWITCH);

    // Once acquired, increment the count
    unsigned long count = GET_READ_ACQUIRE_COUNT(lock);
    SET_READ_ACQUIRE_COUNT(lock, ++count);

    // The first reader must wait for lock, subsequent readers will get access immediately
    if(count == 1) spinlock_acquire(lock);

    // Once acquired release lightswitch lock
    _spinlock_release(lock, SPINLOCK_LIGHTSWITCH);
}

void spinlock_read_release(spinlock_t *lock) {
    // Attempt to set the lightswitch bit
    _spinlock_release(lock, SPINLOCK_LIGHTSWITCH);

    // Once acquired, decrement the count
    unsigned long count = GET_READ_ACQUIRE_COUNT(lock);
    SET_READ_ACQUIRE_COUNT(lock, --count);

    // The last reader must release the spinlock
    if(count == 0) spinlock_release(lock);

    // Release the lightswitch lock
    _spinlock_release(lock, SPINLOCK_LIGHTSWITCH);
}
