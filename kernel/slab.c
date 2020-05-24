#include <kernel/slab.h>
#include <kernel/kassert.h>
#include <string.h>

// Basically, we want to shuffle a slab to be first in the search order if it has 1/4+ (25%+) more free blocks
// than the current first slab in the search order.
#define SLAB_SHUFFLE_SHIFT                  (2)
#define SLAB_SHUFFLE_THRESHOLD(first_slab)  ((first_slab)->free_blocks_remaining + ((first_slab)->capacity >> SLAB_SHUFFLE_SHIFT))

void _slab_shuffle(slab_t **first_slab, slab_t *this_slab, slab_t *prev_slab) {
    prev_slab->next_slab = this_slab->next_slab;
    this_slab->next_slab = *first_slab;
    *first_slab = this_slab;
}

slab_t* slab_init(void *ptr, size_t size, size_t block_size) {
    kassert(ptr != NULL && size > SLAB_MIN_SIZE);

    // The slab struct is placed at the beginning of the region pointed to in ptr
    slab_t *slab = (slab_t*)ptr;
    slab->magic = SLAB_MAGIC;
    slab->next_slab = NULL;
    slab->block_size = block_size;
    slab->capacity = (size - sizeof(slab_t)) / slab->block_size;
    slab->free_blocks_remaining = slab->capacity;

    // Create a pointer chain to link all free blocks
    slab->next_free = ptr + sizeof(slab_t);
    void *next = slab->next_free;
    for (unsigned int i = 0; i < (slab->capacity-1); i++) {
        void *next_block = (void*)((uintptr_t)next + slab->block_size);
        *((uintptr_t*)next) = (uintptr_t)next_block;
        next = next_block;
    }

    // The last block points to NULL
    *((uintptr_t*)next) = 0;

    return slab;
}

void* slab_alloc(slab_t **slab) {
    kassert(slab != NULL && *slab != NULL);

    void *free = NULL;

    for (slab_t *this_slab = *slab, *prev_slab = NULL; this_slab != NULL; prev_slab = this_slab, this_slab = this_slab->next_slab) {
        kassert(this_slab->magic == SLAB_MAGIC);
        kassert(this_slab->free_blocks_remaining <= this_slab->capacity);

        // Check if we have room in this slab, if not continue to the next slab
        if (this_slab->free_blocks_remaining > 0) {
            // Allocate a block from this slab and update the book-keeping
            free = this_slab->next_free;
            this_slab->next_free = (void*)(*((uintptr_t*)free));
            this_slab->free_blocks_remaining--;

            // Update the head slab pointer to point to the latest slab that had free blocks
            // and rearrange the linked list as needed. Basically we want this_slab to be the
            // first slab searched next time alloc is called
            if (prev_slab != NULL) {
                _slab_shuffle(slab, this_slab, prev_slab);
            }

            break;
        }
    }

    return free;
}

void* slab_zalloc(slab_t **slab) {
    // First allocate a block
    void *free = slab_alloc(slab);

    // Check for no free blocks
    if (free != NULL) memset(free, 0, (*slab)->block_size);
    return free;
}

void slab_free(slab_t **slab, void *block) {
    kassert(slab != NULL && *slab != NULL);

    slab_t *this_slab, *prev_slab;
    for (this_slab = *slab, prev_slab = NULL; this_slab != NULL; prev_slab = this_slab, this_slab = this_slab->next_slab) {
        kassert(this_slab->magic == SLAB_MAGIC);
        kassert(this_slab->free_blocks_remaining <= this_slab->capacity);

        // Check if this block is part of this slab, if not continue to the next slab
        intptr_t block_bounds = (intptr_t)block - ((intptr_t)this_slab + sizeof(slab_t));
        if (block_bounds >= 0 && block_bounds < (this_slab->capacity * this_slab->block_size)) {
            // Add this block back into the linked list of free blocks
            *((uintptr_t*)block) = (uintptr_t)this_slab->next_free;
            this_slab->next_free = block;
            this_slab->free_blocks_remaining++;

            // Now determine whether this slab should be placed first in the search order
            // Ideally we want the slab with the most free blocks to be searched first
            if (prev_slab != NULL && this_slab->free_blocks_remaining > SLAB_SHUFFLE_THRESHOLD(*slab)) {
                _slab_shuffle(slab, this_slab, prev_slab);
            }

            break;
        }
    }

    // We must not be trying to free something that was never allocated in this slab
    kassert(this_slab != NULL);
}

void slab_grow(slab_t **slab, void *ptr, size_t size) {
    kassert(slab != NULL && *slab != NULL && (*slab)->magic == SLAB_MAGIC);
    kassert(ptr != NULL);

    slab_t *new_slab = slab_init(ptr, size, (*slab)->block_size);
    new_slab->next_slab = *slab;
    *slab = new_slab;
}

slab_t* slab_shrink(slab_t **slab) {
    kassert(slab != NULL && *slab != NULL);

    slab_t *freed_slab = NULL;
    for (slab_t *this_slab = *slab, *prev_slab = NULL; this_slab != NULL; prev_slab = this_slab, this_slab = this_slab->next_slab) {
        kassert(this_slab->magic == SLAB_MAGIC);
        kassert(this_slab->free_blocks_remaining <= this_slab->capacity);

        // Check if this slab is completely empty, if so we can unlink it from the slab list
        if (this_slab->free_blocks_remaining == this_slab->capacity) {
            freed_slab = this_slab;
            if (prev_slab != NULL) {
                prev_slab->next_slab = this_slab->next_slab;
            } else {
                *slab = this_slab->next_slab;
            }
            break;
        }
    }

    return freed_slab;
}
