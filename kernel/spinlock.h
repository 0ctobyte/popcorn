#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <sys/types.h>

#define SPINLOCK_INIT (0)

typedef long spinlock_t;

// Initializes a spinlock, MUST always be called on a newly declared spinlock
void spinlock_init(spinlock_t *lock);

// Attempt to acquire a spinlock
void spinlock_acquire(spinlock_t *lock);

// Try to acquire the spinlock, this will not "spin" if the locking fails
// Returns true if locking succeeded, otherwise false
bool spinlock_try_acquire(spinlock_t *lock);

// Releases a previously locked spinlock
void spinlock_release(spinlock_t *lock);

// Same as spinlock_acquire except this also disables interrupts
// This should be used if the critical section to be protected can be entered in an interrupt context
void spinlock_irq_acquire(spinlock_t *lock);

// Release a spinlock and, if previously enabled, re-enables interrupts
void spinlock_irq_release(spinlock_t *lock);

// This will allow multiple readers to acquire the spinlock simultaneously
void spinlock_read_acquire(spinlock_t *lock);

// Indicates when a reader is done
void spinlock_read_release(spinlock_t *lock);

// Writers are only allowed access to the critical section only when no other entity
// (readers & writers) has acquired the spinlock
#define spinlock_write_acquire(lock) spinlock_acquire(lock)

// Indicates when a writer is done
#define spinlock_write_release(lock) spinlock_release(lock)

#endif // _SPINLOCK_H_
