#ifndef __KHEAP_H__
#define __KHEAP_H__

#include <sys/types.h>

// TODO: This is temporary for testing
extern uintptr_t bins[20];

// Initialize the kernel heap
void kheap_init();

// Allocate a block of the requested size
void* kheap_alloc(size_t size);

// Frees the block of memory
void kheap_free(void *free);

#endif // __KHEAP_H__

