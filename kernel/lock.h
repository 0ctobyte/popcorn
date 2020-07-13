#ifndef _LOCK_H_
#define _LOCK_H_

#include <sys/types.h>
#include <kernel/spinlock.h>

/* Lock - A general purpose lock supporting exclusive and shared lock ownership.
 * Multiple threads can gain shared ownership of the lock but only one thread can gain exclusive ownership at a time.
 * If a thread requests exclusive ownership while the lock is being shared, it will prevent future shared ownership
 * of the lock from threads with lower priority. Priority inversion is handled for exclusive only ownership of the
 * lock by passing the priority of the blocking thread to the thread that currently owns the lock. The lock will
 * sleep threads if it can't gain ownership and wake threads when the lock is released.
 */

struct proc_thread_s;

/* Valid state transitions
 *                   | FREE | SHARED | EXCLUSIVE | EXCLUSIVE_UPGRADE
 * FREE              |      | X      | X         |
 * SHARED            | X    | X      |           | X
 * EXCLUSIVE         | X    |        |           |
 * EXCLUSIVE_UPGRADE | X    |        |           |
 */
typedef enum {
    LOCK_STATE_FREE,
    LOCK_STATE_SHARED,
    LOCK_STATE_EXCLUSIVE,
    LOCK_STATE_EXCLUSIVE_UPGRADE,
} lock_state_t;

typedef struct {
    spinlock_t interlock;          // Spinlock protecting this lock's data
    struct proc_thread_s *thread;  // Thread requesting or holding exclusive ownership to this lock
    unsigned int shared_count;     // Count of threads sharing this lock
    lock_state_t state;            // Lock state
} lock_t;

#define LOCK_INITIALIZER (lock_t){ .interlock = SPINLOCK_INITIALIZER, .thread = NULL, .shared_count = 0,\
    .state = LOCK_STATE_FREE }

// Initializes a lock, MUST always be called before a lock can be used
#define lock_init(lock)        (*(lock) = LOCK_INITIALIZER)

// Acquire exclusive ownership of the lock sleeping the thread if it can't gain ownership
void lock_acquire_exclusive(lock_t *lock);

// Acquire shared ownership of the lock sleeping the thread if it can't gain ownership
void lock_acquire_shared(lock_t *lock);

// Try to acquire the lock. This will not sleep the thread if acquisition fails. Returns true if lock was acquired
// otherwise returns false
bool lock_try_acquire_exclusive(lock_t *lock);
bool lock_try_acquire_shared(lock_t *lock);

// Release exclusive ownership of the lock waking threads that waiting for it
void lock_release_exclusive(lock_t *lock);

// Release shared ownership of the lock. This will only wake blocked threads if the last shared owner has released it
void lock_release_shared(lock_t *lock);

#define lock_acquire(lock)     lock_acquire_exclusive(lock)
#define lock_try_acquire(lock) lock_try_acquire_exclusive(lock)
#define lock_release(lock)     lock_release_exclusive(lock)

#endif // _LOCK_H_
