#ifndef _KMEM_SLAB_H_
#define _KMEM_SLAB_H_

#include <sys/types.h>
#include <kernel/slab.h>
#include <kernel/lock.h>

/* kmem_slab - General purpose slab allocator using the kernel virtual address space and protected by locks
 * kmem_slab is built on the notion that any kernel module that needs to dynamically allocate memory is responsible
 * for setting up their own slab of objects to allocate from. The modules will handle out of memory conditions
 * themselves (typically by purging least recently used objects and freeing them). kmem_slab will grab free virtual
 * memory from the kernel for new slabs.
 */

// kmem_slab is just a slab with mutex lock using the kernel VM address space
typedef struct {
    lock_t lock;
    slab_t slab;
    slab_buf_t slab_buf;
    size_t size;
} kmem_slab_t;

// Create a new slab. The no_vm version takes the pointer to the slab buffer instead
// of allocating kernel virtual address space for it.
void kmem_slab_create_no_vm(kmem_slab_t *kmem_slab, size_t object_size, size_t num_objects, void *buf);
void kmem_slab_create(kmem_slab_t *kmem_slab, size_t object_size, size_t num_objects);

// Delete an allocation zone
void kmem_slab_destroy(kmem_slab_t *kmem_slab);

// Allocate memory according to the given size. The kmem_slab_zalloc variant zeros out the memory
void* kmem_slab_alloc(kmem_slab_t *kmem_slab);
void* kmem_slab_zalloc(kmem_slab_t *kmem_slab);

// Free the memory previously allocated by kmem_slab_(z)alloc
void kmem_slab_free(kmem_slab_t *kmem_slab, void *ptr);

#endif // _KMEM_SLAB_H_
