#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef struct spinlock spinlock_t;
typedef struct spinlock_rw spinlock_rw_t;

// Initializes a spinlock, MUST always be called on a newly declared spinlock
void spin_init(spinlock_t*);

// Attempt to lock a spinlock
void spin_lock(spinlock_t*);

// Unlock a previously locked spinlock
void spin_unlock(spinlock_t*);

// Same as spin_lock except this also disables interrupts
// This should be used if the object to be locked will be accessed in an 
// interrupt context
void spin_irq_lock(spinlock_t*);

// Unlock a spinlock and, if previously enabled, reenables interrupts
void spin_irq_unlock(spinlock_t*);

#endif // __SPINLOCK_H__

