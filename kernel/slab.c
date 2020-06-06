#include <kernel/slab.h>
#include <kernel/kassert.h>
#include <lib/asm.h>

// Basically, we want to shuffle a slab to be first in the search order if it has 1/4+ (25%+) more free blocks
// than the current first slab in the search order.
#define SLAB_SHUFFLE_SHIFT                  (2)
#define SLAB_SHUFFLE_THRESHOLD(first_slab)  ((first_slab)->free_blocks_remaining + ((first_slab)->capacity >> SLAB_SHUFFLE_SHIFT))

void _slab_shuffle(slab_t *slab, slab_buf_t *this_slab_buf, slab_buf_t *prev_slab_buf) {
    prev_slab_buf->next_slab = this_slab_buf->next_slab;
    this_slab_buf->next_slab = slab->first;
    slab->first = this_slab_buf;
}

slab_buf_t* _slab_buf_init(slab_buf_t *slab_buf, void *buf, size_t size, size_t block_size) {
    // The slab_buf struct is placed at the beginning of the region pointed to in buf if it's not provided
    if (slab_buf == NULL) slab_buf = (slab_buf_t*)buf;

    // Adjust capacity and free pointer chain if slab_buf_t is embedded within the actual slab buffer
    size_t offset = ((uintptr_t)slab_buf == (uintptr_t)buf) ? sizeof(slab_buf_t) : 0;

    slab_buf->buf = buf;
    slab_buf->next_free = NULL;
    slab_buf->next_slab = NULL;
    slab_buf->capacity = (size - offset) / block_size;
    slab_buf->free_blocks_remaining = slab_buf->capacity;

    // Create a pointer chain to link all free blocks
    slab_buf->next_free = buf + offset;
    void *next = slab_buf->next_free;

    for (unsigned int i = 0; i < slab_buf->free_blocks_remaining; i++) {
        void *next_block = (void*)((uintptr_t)next + block_size);
        *((uintptr_t*)next) = (uintptr_t)next_block;
        next = next_block;
    }

    // The last block points to NULL
    *((uintptr_t*)next) = 0;

    return slab_buf;
}

void slab_init(slab_t *slab, slab_buf_t *slab_buf, void *buf, size_t size, size_t block_size) {
    kassert(slab != NULL && buf != NULL && size > SLAB_MIN_SIZE);

    _slab_buf_init(slab_buf, buf, size, block_size);

    slab->first = slab_buf;
    slab->block_size = block_size;
}

void* slab_alloc(slab_t *slab) {
    kassert(slab != NULL && slab->first != NULL);

    void *free = NULL;

    for (slab_buf_t *this_slab_buf = slab->first, *prev_slab_buf = NULL; this_slab_buf != NULL; prev_slab_buf = this_slab_buf, this_slab_buf = this_slab_buf->next_slab) {
        kassert(this_slab_buf->free_blocks_remaining <= this_slab_buf->capacity);

        // Check if we have room in this slab, if not continue to the next slab
        if (this_slab_buf->free_blocks_remaining > 0) {
            // Allocate a block from this slab and update the book-keeping
            free = this_slab_buf->next_free;
            this_slab_buf->next_free = (void*)(*((uintptr_t*)free));
            this_slab_buf->free_blocks_remaining--;

            // Update the first slab buf pointer to point to the latest slab that had free blocks and rearrange the linked list as needed.
            // Basically we want this_slab_buf to be the first slab searched next time alloc is called
            if (prev_slab_buf != NULL) {
                _slab_shuffle(slab, this_slab_buf, prev_slab_buf);
            }

            break;
        }
    }

    return free;
}

void* slab_zalloc(slab_t *slab) {
    // First allocate a block
    void *free = slab_alloc(slab);

    // Check for no free blocks
    if (free != NULL) _fast_zero((uintptr_t)free, slab->block_size);
    return free;
}

void slab_free(slab_t *slab, void *block) {
    kassert(slab != NULL && slab->first != NULL);

    slab_buf_t *this_slab_buf, *prev_slab_buf;
    for (this_slab_buf = slab->first, prev_slab_buf = NULL; this_slab_buf != NULL; prev_slab_buf = this_slab_buf, this_slab_buf = this_slab_buf->next_slab) {
        kassert(this_slab_buf->free_blocks_remaining <= this_slab_buf->capacity);

        // Offset the beginning of the slab buffer if the slab_buf_t struct is embedded within the buffer
        size_t offset = ((uintptr_t)this_slab_buf == (uintptr_t)this_slab_buf->buf) ? sizeof(slab_buf_t) : 0;

        // Check if this block is part of this slab, if not continue to the next slab
        intptr_t block_bounds = (intptr_t)block - ((intptr_t)this_slab_buf->buf + offset);
        if (block_bounds >= 0 && block_bounds < (this_slab_buf->capacity * slab->block_size)) {
            // Add this block back into the linked list of free blocks
            *((uintptr_t*)block) = (uintptr_t)this_slab_buf->next_free;
            this_slab_buf->next_free = block;
            this_slab_buf->free_blocks_remaining++;

            // Now determine whether this slab should be placed first in the search order
            // Ideally we want the slab with the most free blocks to be searched first
            if (prev_slab_buf != NULL && this_slab_buf->free_blocks_remaining > SLAB_SHUFFLE_THRESHOLD(slab->first)) {
                _slab_shuffle(slab, this_slab_buf, prev_slab_buf);
            }

            break;
        }
    }

    // We must not be trying to free something that was never allocated in this slab
    kassert(this_slab_buf != NULL);
}

void slab_grow(slab_t *slab, slab_buf_t *new_slab_buf, void *buf, size_t size) {
    kassert(slab != NULL && slab->first != NULL && buf != NULL);

    _slab_buf_init(new_slab_buf, buf, size, slab->block_size);

    new_slab_buf->next_slab = slab->first;
    slab->first = new_slab_buf;
}

void* slab_shrink(slab_t *slab) {
    kassert(slab != NULL && slab->first != NULL);

    slab_buf_t *freed_slab = NULL;
    for (slab_buf_t *this_slab_buf = slab->first, *prev_slab_buf = NULL; this_slab_buf != NULL; prev_slab_buf = this_slab_buf, this_slab_buf = this_slab_buf->next_slab) {
        kassert(this_slab_buf->free_blocks_remaining <= this_slab_buf->capacity);

        // Check if this slab is completely full, if so we can unlink it from the slab list
        if (this_slab_buf->free_blocks_remaining == this_slab_buf->capacity) {
            freed_slab = this_slab_buf;
            if (prev_slab_buf != NULL) {
                prev_slab_buf->next_slab = this_slab_buf->next_slab;
            } else {
                slab->first = this_slab_buf->next_slab;
            }
            break;
        }
    }

    return (void*)freed_slab;
}
