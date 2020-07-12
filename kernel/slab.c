#include <kernel/kassert.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/slab.h>

// Basically, we want to shuffle a slab to be first in the search order if it has 1/4+ (25%+) more free blocks
// than the current first slab in the search order.
#define SLAB_SHUFFLE_SHIFT                  (2)
#define SLAB_SHUFFLE_THRESHOLD(first_slab)  ((first_slab)->free_blocks_remaining +\
    ((first_slab)->capacity >> SLAB_SHUFFLE_SHIFT))

void _slab_shuffle(slab_t *slab, slab_buf_t *this_slab_buf) {
    kassert(list_remove(&slab->ll_slabs, &this_slab_buf->ll_node));
    kassert(list_push(&slab->ll_slabs, &this_slab_buf->ll_node));
}

list_compare_result_t _slab_buf_find(list_node_t *n1, list_node_t *n2) {
    slab_buf_t *s2 = list_entry(n2, slab_buf_t, ll_node);
    return (s2->free_blocks_remaining > 0) ? LIST_COMPARE_EQ : LIST_COMPARE_LT;
}

list_compare_result_t _slab_buf_find_full(list_node_t *n1, list_node_t *n2) {
    slab_buf_t *s2 = list_entry(n2, slab_buf_t, ll_node);
    return slab_buf_is_full(s2) ? LIST_COMPARE_EQ : LIST_COMPARE_LT;
}

list_compare_result_t _slab_buf_compare(list_node_t *n1, list_node_t *n2) {
    slab_buf_t *s2 = list_entry(n2, slab_buf_t, ll_node);
    size_t offset = ((uintptr_t)s2 == (uintptr_t)s2->buf) ? sizeof(slab_buf_t) : 0, block_size = *((size_t*)n1);

    uintptr_t buf_start = (uintptr_t)s2->buf + offset, buf_end = buf_start + (s2->capacity * block_size);
    uintptr_t block_start = (uintptr_t)n1, block_end = block_start + block_size;

    return (block_start < buf_start) ? LIST_COMPARE_LT : (block_end > buf_end) ? LIST_COMPARE_GT : LIST_COMPARE_EQ;
}

slab_buf_t* _slab_buf_init(slab_buf_t *slab_buf, void *buf, size_t size, size_t block_size) {
    // The slab_buf struct is placed at the beginning of the region pointed to in buf if it's not provided
    if (slab_buf == NULL) slab_buf = (slab_buf_t*)buf;

    // Adjust capacity and free pointer chain if slab_buf_t is embedded within the actual slab buffer
    size_t offset = ((uintptr_t)slab_buf == (uintptr_t)buf) ? sizeof(slab_buf_t) : 0;

    slab_buf->buf = buf;
    list_init(&slab_buf->ll_free);
    list_node_init(&slab_buf->ll_node);
    slab_buf->capacity = (size - offset) / block_size;
    slab_buf->free_blocks_remaining = slab_buf->capacity;

    // Create a pointer chain to link all free blocks
    list_node_t *next_free = (list_node_t*)(buf + offset);
    for (unsigned int i = 0; i < slab_buf->free_blocks_remaining; i++) {
        list_node_init(next_free);
        list_push(&slab_buf->ll_free, next_free);
        next_free = (list_node_t*)((uintptr_t)next_free + block_size);
    }

    return slab_buf;
}

void slab_init(slab_t *slab, slab_buf_t *slab_buf, void *buf, size_t size, size_t block_size) {
    kassert(slab != NULL && buf != NULL && size > SLAB_MIN_SIZE);

    slab_buf = _slab_buf_init(slab_buf, buf, size, block_size);

    list_init(&slab->ll_slabs);
    list_push(&slab->ll_slabs, &slab_buf->ll_node);
    slab->block_size = block_size;
}

void* slab_alloc(slab_t *slab) {
    kassert(slab != NULL && list_first(&slab->ll_slabs) != NULL);

    void *free = NULL;

    // Search for a slab buf with free blocks
    slab_buf_t *this_slab_buf = list_entry(list_search(&slab->ll_slabs, _slab_buf_find, NULL), slab_buf_t, ll_node);

    if (this_slab_buf != NULL) {
        kassert(this_slab_buf->free_blocks_remaining <= this_slab_buf->capacity);

        // Allocate a block from this slab and update the book-keeping
        list_node_t *free_node;
        kassert(list_pop(&this_slab_buf->ll_free, free_node));

        free = (void*)free_node;
        this_slab_buf->free_blocks_remaining--;

        // Update the first slab buf pointer to point to the latest slab that had free blocks and rearrange the linked
        // list as needed. Basically we want this_slab_buf to be the first slab searched next time alloc is called
        if ((&this_slab_buf->ll_node) != list_first(&slab->ll_slabs)) _slab_shuffle(slab, this_slab_buf);
    }

    return free;
}

void* slab_zalloc(slab_t *slab) {
    // First allocate a block
    void *free = slab_alloc(slab);

    // Check for no free blocks
    if (free != NULL) arch_fast_zero(free, slab->block_size);
    return free;
}

void slab_free(slab_t *slab, void *block) {
    kassert(slab != NULL && list_first(&slab->ll_slabs) != NULL);

    // Search for the slab buf that contains this block
    *((size_t*)block) = slab->block_size;
    slab_buf_t *this_slab_buf = list_entry(list_search(&slab->ll_slabs, _slab_buf_compare, (list_node_t*)block),
        slab_buf_t, ll_node);

    // We must not be trying to free something that was never allocated in this slab
    kassert(this_slab_buf != NULL);
    kassert(this_slab_buf->free_blocks_remaining <= this_slab_buf->capacity);

    // Add this block back into the linked list of free blocks
    list_node_init((list_node_t*)block);
    kassert(list_push(&this_slab_buf->ll_free, (list_node_t*)block));

    // Now determine whether this slab should be placed first in the search order
    // Ideally we want the slab with the most free blocks to be searched first
    slab_buf_t *first = list_entry(list_first(&slab->ll_slabs), slab_buf_t, ll_node);
    if (list_prev(&this_slab_buf->ll_node) != NULL &&
        this_slab_buf->free_blocks_remaining > SLAB_SHUFFLE_THRESHOLD(first)) {
        _slab_shuffle(slab, this_slab_buf);
    }
}

void slab_grow(slab_t *slab, slab_buf_t *new_slab_buf, void *buf, size_t size) {
    kassert(slab != NULL && list_first(&slab->ll_slabs) != NULL && buf != NULL);

    new_slab_buf = _slab_buf_init(new_slab_buf, buf, size, slab->block_size);
    list_push(&slab->ll_slabs, &new_slab_buf->ll_node);
}

slab_buf_t* slab_shrink(slab_t *slab) {
    if (slab == NULL || list_first(&slab->ll_slabs) == NULL) return NULL;

    // Find a slab that is completely full, if one is found remove it from the slab list
    slab_buf_t *freed_slab = list_entry(list_search(&slab->ll_slabs, _slab_buf_find_full, NULL), slab_buf_t, ll_node);

    if (freed_slab != NULL) {
        kassert(freed_slab->free_blocks_remaining <= freed_slab->capacity);
        kassert(list_remove(&slab->ll_slabs, &freed_slab->ll_node));
    }

    return freed_slab;
}
