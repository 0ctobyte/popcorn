#ifndef __KASSERT_H__
#define __KASSERT_H__

#include <kernel/panic.h>

#define kassert(b) (b) ? (void)0 : \
    panic("Assertion failed: %s:%d: '%s'\n", __FILE__, __LINE__, #b)

#endif // __KASSERT_H__
