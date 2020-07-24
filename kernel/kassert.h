/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _KASSERT_H_
#define _KASSERT_H_

#include <kernel/panic.h>

#define kassert(b) (b) ? (void)0 : \
    panic("Assertion failed: %s:%d: '%s'\n", __FILE__, __LINE__, #b)

#endif // _KASSERT_H_
