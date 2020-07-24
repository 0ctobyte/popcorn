/*
 * Copyright (c) 2015 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

long sprintf(char *s, const char *fmt, ...) {
    va_list args;
    long i = 0;

    va_start(args, fmt);
    i = vsprintf(s, fmt, args);
    va_end(args);

    return(i);
}

