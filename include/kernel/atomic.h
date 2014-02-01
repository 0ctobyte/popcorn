#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <sys/types.h>

// Atomically tests if value at ptr is 0 and if so sets it to 1
// Returns 0 on success, 1 on failure to set
uint32_t atomic_test_and_set(uint32_t *ptr);

#endif // __ATOMIC_H__

