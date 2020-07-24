/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _KMEM_H_
#define _KMEM_H_

#include <sys/types.h>

/*
 * kmem - General purpose kernel memory allocator (CURRENTLY UNUSED)
 * kmem is built on top of slabs, in fact it allocates slabs for each of the power of 2 block sizes up to some maximum
 * (i.e. 64KB). The minimum block size it can allocate is 32 bytes so anything smaller will waste space. It will round
 * up allocation requests to the nearest power of 2 block size and allocate a block from the slab holding those blocks.
 * It has an interesting mechanism when slabs run out of free space; it keeps track of least recently used slabs and
 * steals buffers from them for the slab that ran out of blocks. It will do this as long as buffers can be stolen.
 * Eventually it will ask the VM system for more pages mapped into the kernel virtual address space.
 */

// Initialize the kmem memory allocator syb-system
void kmem_init(void);

// Allocate memory according to the given size. The kmem_zalloc variant zeros out the memory
void* kmem_alloc(size_t size);
void* kmem_zalloc(size_t size);

// Free the memory previously allocated by kmem_(z)alloc
void kmem_free(void *mem, size_t size);

// Print some stats
void kmem_stats(void);

#endif // _KMEM_H_
