/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _CONSOLE_TYPES_H_
#define _CONSOLE_TYPES_H_

#include <sys/types.h>

typedef struct {
    // Initializes the console device
    void (*init)(void*);

    // Writes the given number of characters to the console device
    // Returns the number of consecutive characters written
    int (*write)(void*, const char*, size_t);

    // Reads the given number of characters from the console device
    // Returns the number of characters read
    int (*read)(void*, char*, size_t);
} console_dev_ops_t;

typedef struct {
    void *data;            // Device specific data;
    console_dev_ops_t *ops; // Device operations
} console_dev_t;

extern console_dev_t console_dev;

#endif // _CONSOLE_TYPES_H_
