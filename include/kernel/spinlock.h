#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <sys/types.h>

#define SPINLOCK_INIT (0)

typedef long spinlock_t;

// Initializes a spinlock, MUST always be called on a newly declared spinlock
void spinlock_init(spinlock_t *lock);

// Attempt to lock a spinlock
void spinlock_acquire(spinlock_t *lock);

// Try to lock the spinlock, this will not "spin" if the locking fails
// Returns true if locking succeeded, otherwise false
bool spinlock_tryacquire(spinlock_t *lock);

// Unlock a previously locked spinlock
void spinlock_release(spinlock_t *lock);

// Same as spinlock_lock except this also disables interrupts
// This should be used if the object to be locked will be accessed in an
// interrupt context
void spinlock_irqacquire(spinlock_t *lock);

// Unlock a spinlock and, if previously enabled, reenables interrupts
void spinlock_irqrelease(spinlock_t *lock);

// Read lock, this will allow multiple sources to lock the spinlock
// assuming those sources will only read the locked resource
void spinlock_readacquire(spinlock_t *lock);

// Indicates when a reader is done using the resource protected by the spinlock
void spinlock_readrelease(spinlock_t *lock);

// Write lock, will only allow a single source to access resource and block
// all other sources regardless of whether they will be reading or writing
// resource
void spinlock_writeacquire(spinlock_t *lock);

// Indicates when a writer is done using the resource protected by the spinlock
void spinlock_writerelease(spinlock_t *lock);

#endif // __SPINLOCK_H__
