#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <sys/types.h>

typedef uint32_t atomic_t;

// Atomically tests if value at ptr is 0 and if so sets it to val
// Returns 0 on success, otherwise failure to set
uint32_t atomic_test_and_set(atomic_t *ptr, uint32_t val);

// Atomically tests specified bit in the value at ptr is 0 and if so sets it to 1
// Returns 0 on success, otherwise failure to set
uint32_t atomic_test_and_set_bit(atomic_t *ptr, uint32_t bit);

// Atomically set to the specified value
void atomic_set(atomic_t *v, uint32_t val);

// Atomically increment the value
void atomic_inc(atomic_t *v);

// Atomically decrement the value
void atomic_dec(atomic_t *v);

#endif // __ATOMIC_H__

