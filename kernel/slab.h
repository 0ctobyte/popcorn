/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SLAB_H_
#define _SLAB_H_

#include <sys/types.h>
#include <kernel/list.h>

// Slabs must at least be this size
#define SLAB_MIN_SIZE                     (sizeof(slab_buf_t)+sizeof(list_node_t))
#define SLAB_BUF_SIZE(slab, slab_buf)     (((slab_buf)->capacity * (slab)->block_size) + (((void*)(slab_buf) ==\
    (slab_buf)->buf) ? sizeof(slab_buf_t) : 0))

typedef struct {
    void *buf;                      // Pointer to the buffer this slab_buf struct is managing
    list_node_t ll_node;            // Slab buffer linked list
    list_t ll_free;                 // Linked list of free blocks
    size_t capacity;                // Total # of elements in this slab buffer
    size_t free_blocks_remaining;   // Free blocks remaining in this slab buffer
} slab_buf_t;

typedef struct {
    list_t ll_slabs;    // Linked list of slab buffers
    size_t block_size;  // Size of a single block
} slab_t;

#define slab_buf_is_full(slab_buf) ((slab_buf)->capacity == (slab_buf)->free_blocks_remaining)

// Initialize a slab using the buffer given buf of the specified size and block_size
// If slab_buf is null, the slab_buf_t struct will be embedded within the buf
void slab_init(slab_t *slab, slab_buf_t *slab_buf, void *buf, size_t size, size_t block_size);

// Allocate and free a block. slab_zalloc is used to allocate a zero'd out block
void* slab_alloc(slab_t *slab);
void* slab_zalloc(slab_t *slab);
void slab_free(slab_t *slab, void *block);

// Routines to grow slabs by linking new slab bufs and shrink slabs by unlinking full slab bufs
// slab_shrink returns the pointer to the slab buf that was removed
// If a slab_buf_t struct is provided, that will be used instead of embedding the slab_buf_t struct within the buf
void slab_grow(slab_t *slab, slab_buf_t *new_slab_buf, void *buf, size_t size);
slab_buf_t* slab_shrink(slab_t *slab);

#endif // _SLAB_H_
