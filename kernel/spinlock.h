/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <sys/types.h>

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
