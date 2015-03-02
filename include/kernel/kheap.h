#ifndef __KHEAP_H__
#define __KHEAP_H__

#include <sys/types.h>

// Initialize the kernel heap
void kheap_init();

// Allocate a block of the requested size
void* kheap_alloc(size_t size);

// Frees the block of memory
void kheap_free(void *free);

void kheap_stats(size_t *heap_free, size_t *heap_allocated, size_t *num_free_blocks, size_t *num_used_blocks);

#endif // __KHEAP_H__

