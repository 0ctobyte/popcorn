/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _HASH_H_
#define _HASH_H_

#include <sys/types.h>

#define HASH_FNV_OFFSET_BASIS (0xcbf29ce484222325)
#define HASH_FNV_PRIME        (0x100000001b3)

static inline uint64_t hash64_fnv1a_pair(uint64_t a, uint64_t b) {
    uint64_t hash = HASH_FNV_OFFSET_BASIS;

    hash ^= (a & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 8) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 16) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 24) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 32) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 40) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 48) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((a >> 56) & 0xff);
    hash *= HASH_FNV_PRIME;

    hash ^= (b & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 8) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 16) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 24) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 32) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 40) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 48) & 0xff);
    hash *= HASH_FNV_PRIME;
    hash ^= ((b >> 56) & 0xff);
    hash *= HASH_FNV_PRIME;

    return hash;
}

static inline uint64_t hash64_fnv1a_str(char *s, size_t len) {
    uint64_t hash = HASH_FNV_OFFSET_BASIS;

    for (unsigned int i = 0; i < len; i++) {
        uint8_t byte = (uint8_t)s[i];

        hash ^= byte;
        hash *= HASH_FNV_PRIME;
    }
}

#endif // _HASH_H_
