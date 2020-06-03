#ifndef __KMEM_H__
#define __KMEM_H__

#include <sys/types.h>

// Initialize the kmem memory allocator syb-system
void kmem_init(void);

// Allocate memory according to the given size. The kmem_zalloc variant zeros out the memory
void* kmem_alloc(size_t size);
void* kmem_zalloc(size_t size);

// Free the memory previously allocated by kmem_(z)alloc
void kmem_free(void *mem);

#endif // __KMEM_H__
