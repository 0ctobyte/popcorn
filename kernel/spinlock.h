/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <sys/types.h>

/*
 * spinlock - A typical spinlock facility
 * Spinlocks are basically busywait mutual exclusion locks. When a lock cannot be acquired, the thread will
 * continuously attempt to gain the lock. Spinlocks should only be used if the lock will only be acquired for a small
 * amount of time or if the thread can not be put to sleep while waiting for access to the critical section.
 *
 * This spinlock implementation allows for readers/writers locking. This means that threads that only intend on reading
 * from a resource in memory may do so simultaneously while threads that intend on writing to the resource may only do
 * so one at a time blocking all other threads from accessing the resource. Note: this implementation gives priority to
 * reader threads which may cause writer threads to starve.
 *
 * The spinlock uses only one integer where certain bits are used for certain purposes. Bit 0 is used as the main lock.
 * Bit 1 is used to hold the state of interrupts, if 1 then interrupts were previously enabled before the call to
 * acquire_irq. Bit 2 is used as the "lightswitch" lock. This is only used when using acquire_read/acquire_release
 * functions. It basically allows only one thread to modify the "count" (count of how many readers have acquired a
 * lock) which starts from bit 3 in the lock variable.
 */

#define SPINLOCK_INITIALIZER (0)

typedef long spinlock_t;

// Initializes a spinlock, MUST always be called on a newly declared spinlock
#define spinlock_init(lock) (*(lock) = SPINLOCK_INITIALIZER)

// Attempt to acquire a spinlock
void spinlock_acquire(spinlock_t *lock);

// Try to acquire the spinlock, this will not "spin" if the locking fails
// Returns true if locking succeeded, otherwise false
bool spinlock_acquire_try(spinlock_t *lock);

// Releases a previously locked spinlock
void spinlock_release(spinlock_t *lock);

// Same as spinlock_acquire except this also disables interrupts
// This should be used if the critical section to be protected can be entered in an interrupt context
void spinlock_acquire_irq(spinlock_t *lock);

// Release a spinlock and, if previously enabled, re-enables interrupts
void spinlock_release_irq(spinlock_t *lock);

// This will allow multiple readers to acquire the spinlock simultaneously
void spinlock_acquire_read(spinlock_t *lock);

// Indicates when a reader is done
void spinlock_release_read(spinlock_t *lock);

// Writers are only allowed access to the critical section only when no other entity
// (readers & writers) has acquired the spinlock
#define spinlock_acquire_write(lock) spinlock_acquire(lock)

// Indicates when a writer is done
#define spinlock_release_write(lock) spinlock_release(lock)

#endif // _SPINLOCK_H_
