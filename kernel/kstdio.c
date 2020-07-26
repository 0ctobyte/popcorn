/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <kernel/kresult.h>
#include <kernel/console.h>
#include <kernel/lock.h>
#include <kernel/kstdio.h>

typedef struct {
    lock_t lock;
    char buffer[1024];
} kstdio_t;

kstdio_t kstdio = { .lock = LOCK_INITIALIZER, .buffer = {0}};

int kputs(const char *s) {
    size_t len = strlen(s), count = 0;
    kresult_t res;

    do {
        size_t c = 0;
        res = console_write(s + count, len - count, &c);
        count += c;
    } while (res == KRESULT_OK && count < len);

    return 0;
}

int kprintf(const char *fmt, ...) {
    va_list args;
    int r = 0;

    lock_acquire_exclusive(&kstdio.lock);

    va_start(args, fmt);
    r = vsprintf(kstdio.buffer, fmt, args);
    va_end(args);

    kputs(kstdio.buffer);

    lock_release_exclusive(&kstdio.lock);

    return r;
}
