#ifndef __SLAB_H__
#define __SLAB_H__

#include <sys/types.h>

// Slabs must at least be this size
#define SLAB_MIN_SIZE  (sizeof(slab_t))

#define SLAB_MAGIC     (0xFEED51ABFEED51AB)

typedef struct slab_s {
    unsigned long magic;             // Magic value to make sure SLAB is valid/not corrupted
    void *next_free;                // Pointer to next free block
    struct slab_s *next_slab;       // Linked list of slabs as they are added
    size_t capacity;                // Total # of elements in this block
    size_t free_blocks_remaining;   // Free blocks remaining in this slab
    size_t block_size;              // Size of a single block
} slab_t;

// Create a slab, size is the total size of the slab to manage and block_size is the size of a single allocatable element
slab_t* slab_init(void *ptr, size_t size, size_t block_size);

// Allocate and free a block. slab_zalloc is used to allocate a zero'd out block
// These functions take a pointer to the slab pointer because they may update the slab pointer when it shuffles multiple
// slabs around. Slab shuffling occurs so that slabs with remaining space are searched first.
void* slab_alloc(slab_t **slab);
void* slab_zalloc(slab_t **slab);
void slab_free(slab_t **slab, void *block);

// Routines to grow slabs by linking new memory regions and shrink slabs by unlinking empty regions
// slab_shrink returns the pointer to the slab block that was removed
void slab_grow(slab_t **slab, void *ptr, size_t size);
slab_t* slab_shrink(slab_t **slab);

#endif // __SLAB_H__
