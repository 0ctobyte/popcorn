/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <kernel/kresult.h>
#include <kernel/console_types.h>

/*
 * console - Common generic console interface for a variety of console devices
 * This is mostly needed for early kernel initialization and for printing errors when no display drivers are available.
 * A console device may or may not actually be connected/setup by platform code. In that case the functions here won't
 * do anything.
 */

// Kernel interface
kresult_t console_init(void);

// The number of characters that need to read or written is provided in len
// the number of characters that were actually read/written is returned is count
kresult_t console_write(const char *src, size_t len, size_t *count);
kresult_t console_read(char *dst, size_t len, size_t *count);

#endif // _CONSOLE_H_
