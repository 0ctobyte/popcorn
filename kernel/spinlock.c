/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/arch/arch_atomic.h>
#include <kernel/arch/arch_barrier.h>
#include <kernel/arch/arch_interrupts.h>
#include <kernel/spinlock.h>

// Meaning of the bits in the spinlock's lock variable
// Bit 0 is the lock value: 0 for unlocked, 1 for acquired
#define SPINLOCK_ACQUIRED              (0x1UL)
// Bit 1 is the status of interrupts: 1 for enabled, 0 for disabled
#define SPINLOCK_ENABLED               (0x2UL)
// Bit 2 is for the R/W value when using readacquire and writeacquire
#define SPINLOCK_LIGHTSWITCH           (0x4UL)

// The counter for the number of readers holding the lock is stored starting from bit 3 in the lock variable
#define GET_READ_ACQUIRE_COUNT(B)      (*(B) >> 3)
#define SET_READ_ACQUIRE_COUNT(B, val) (*(B) |= ((val) << 3))

// Set enabled bit, get enabled bit
#define GET_ENABLED_BIT(B)             (*(B) & SPINLOCK_ENABLED)
#define SET_ENABLED_BIT(B, val)        (*(B) |= ((val) << 1))

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

bool spinlock_acquire_try(spinlock_t *lock) {
    return !arch_atomic_test_and_set_bit(lock, SPINLOCK_ACQUIRED);
}

void spinlock_acquire_irq(spinlock_t *lock) {
    // Get state of interrupts
    bool _enabled = arch_interrupts_is_enabled();
    arch_interrupts_disable();
    spinlock_acquire(lock);

    // Set enabled bit to 1 if interrupts were enabled
    SET_ENABLED_BIT(lock, _enabled);
}

void spinlock_release_irq(spinlock_t *lock) {
    // Get the previous interrupt enabled state
    bool _enabled = GET_ENABLED_BIT(lock);
    spinlock_release(lock);
    if(_enabled) arch_interrupts_enable();
}

void spinlock_acquire_read(spinlock_t *lock) {
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

void spinlock_release_read(spinlock_t *lock) {
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
