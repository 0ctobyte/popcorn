#ifndef __SLAB_H__
#define __SLAB_H__

#include <sys/types.h>

// Slabs must at least be this size
#define SLAB_MIN_SIZE  (sizeof(slab_t))

#define SLAB_MAGIC     (0xFEED51ABFEED51AB)

typedef struct slab_buf_s {
    unsigned long magic;            // Magic value to make sure slab buffer is valid/not corrupted
    void *next_free;                // Pointer to next free block
    struct slab_buf_s *next_slab;   // Next slab buffer in the linked list of slab buffers
    size_t capacity;                // Total # of elements in this slab buffer
    size_t free_blocks_remaining;   // Free blocks remaining in this slab buffer
} slab_buf_t;

typedef struct slab_s {
    slab_buf_t *first;  // First slab in the linked list of slab buffers
    size_t block_size;  // Size of a single block
} slab_t;

// Initialize a slab using the buffer given in ptr of the specified size and block_size
void slab_init(slab_t *slab, void *ptr, size_t size, size_t block_size);

// Allocate and free a block. slab_zalloc is used to allocate a zero'd out block
void* slab_alloc(slab_t *slab);
void* slab_zalloc(slab_t *slab);
void slab_free(slab_t *slab, void *block);

// Routines to grow slabs by linking new slab bufs and shrink slabs by unlinking full slab bufs
// slab_shrink returns the pointer to the slab buf that was removed
void slab_grow(slab_t *slab, void *ptr, size_t size);
void* slab_shrink(slab_t *slab);

#endif // __SLAB_H__
