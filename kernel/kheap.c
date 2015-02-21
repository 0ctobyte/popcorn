#include <kernel/kheap.h>
#include <kernel/vmm.h>
#include <kernel/kassert.h>

#include <lib/asm.h>

#include <string.h>

// Assume all numbers are 32-bit numbers
#define BITS (32)

// Maximum block size allowed 
// Minimum block size: 4 bytes
#define NUM_BINS (20) // 2 MB
#define MIN_BLOCK_SIZE (0x4)
#define MAX_BLOCK_SIZE ((uint32_t)((1 << _ctz(MIN_BLOCK_SIZE)) << (NUM_BINS-1)))

// True if n is a power of 2, else false
#define IS_POW2(n) (((n) & ((n)-1)) == 0)

// Round to next power of 2
#define ROUND_NEXT_POW2(n) (IS_POW2((n)) ? (n) : ((~(1 << (((BITS-1) - _clz((n))) - 1)) & (n)) << 1))

// Make sure the size is at least MIN_BLOCK_SIZE
#define CONSTRAIN_TO_MIN_BLOCK_SIZE(n) (((n) < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : (n))

// Given a power of 2 #, return the index into the bin array
#define GET_BIN_INDEX(n) ((BITS-1) - (_clz(((n) >> (_ctz(MIN_BLOCK_SIZE))))))

// Given the address of the block and size, returns the address of it's buddy
#define GET_BUDDY(f, s) ((((f)-vmap_kernel()->heap_start) ^ (s)) + vmap_kernel()->heap_start)

/*
 * TODO: I was tired and weak when I wrote this commentary. Make this better pls.
 * Buddy allocation based heap memory manager.
 * 
 * This algorithm works by having bins storing free blocks of a certain size (sizes are power of 2s).
 * To allocate a block of size s, s must be rounded up to the next power of 2. Then, the bin for that size 
 * is searched for a free block. If one exist, the address of that block is returned. If none exist, then
 * the bin for the next power of 2 size is searched. If one exists, then the block is split in half, with one half
 * inserted into the bin of size s (rounded up to the next power of 2) and the other returned (allocated). If there is 
 * no block available in the next bin, then this process is repeated until a block is found in a higher up bin and halved
 * recursively until a block of size NEXT_POW2(s) is available.
 *
 * Every allocated block will have a kheap_ublock_t structure allocated on the heap (thus every allocation request
 * will need to allocate a block of the requested size AND allocate a kheap_ublock_t struct). The kheap_ublock_t structs are linked
 * together in a list. To a free a block, the kheap_ublock_t list is searched for the kheap_ublock_t struct that contains the pointer
 * of the to-be-freed address. If found, the block is placed the appropriate bin and merged with any buddies recursively
 * Then the kheap_ublock_t struct is unlinked from the list and placed in the appropriate bin and merged recursively with any buddies.
 * Merging works by checking if the block has any buddies in the bin of the block's size. If a buddy exist it is merged and placed
 * in the next higher up bin. This process is repeated until the block has no buddies in the bin.
 */

typedef struct kheap_ublock {
  size_t size;
  vaddr_t addr;
  struct kheap_ublock *next;
  struct kheap_ublock *prev;
} kheap_ublock_t;

vaddr_t bins[NUM_BINS] = {0};
static kheap_ublock_t *root;

void _kheap_bin_insert(vaddr_t free, size_t block_size) {
  kassert(IS_POW2(block_size) && (block_size <= MAX_BLOCK_SIZE) && (block_size >= MIN_BLOCK_SIZE));

  uint32_t index = GET_BIN_INDEX(block_size);
  vaddr_t head = bins[index];

  // The list of free blocks is sorted by increasing address
  vaddr_t next, prev = 0, prev_prev = 0;
  for(next = head; next != 0; next = *(vaddr_t*)next) {
    if(free < next) break;
    prev_prev = prev;
    prev = next;
  }

  // Is prev or next a buddy of this block? If so merge them. We can't merge blocks that are MAX_BLOCK_SIZE
  vaddr_t buddy = ((block_size == MAX_BLOCK_SIZE) ? free : GET_BUDDY(free, block_size)); 
  if(buddy == prev) {
    // prev block is a buddy
    if(prev_prev != 0) *((vaddr_t*)prev_prev) = next;
    else bins[index] = next;
    *((vaddr_t*)prev) = 0;
    _kheap_bin_insert(prev, block_size << 1);
  } else if(buddy == next) {
    // next block is a buddy
    vaddr_t next_next = *((vaddr_t*)next);
    if(prev != 0) *((vaddr_t*)prev) = next_next;
    else bins[index] = next_next;
    *((vaddr_t*)next) = 0;
    _kheap_bin_insert(free, block_size << 1);
  } else {
    // No buddies, just insert here
    *((vaddr_t*)free) = next;
    if(prev != 0) *((vaddr_t*)prev) = free;
    else bins[index] = free;
  }
}

vaddr_t _kheap_bin_pop(size_t block_size) {
  kassert(IS_POW2(block_size) && (block_size <= MAX_BLOCK_SIZE) && (block_size >= MIN_BLOCK_SIZE));

  uint32_t index = GET_BIN_INDEX(block_size);
  vaddr_t head = bins[index];
 
  // Unlink it from the bin if the block exists
  if(head != 0) {
    bins[index] = *(vaddr_t*)head;
    memset((void*)head, 0, block_size);
  }

  return head;
}

vaddr_t _kheap_get_free_block(size_t block_size) {
  // Get the first block in the bin
  vaddr_t ptr = _kheap_bin_pop(block_size);

  // If there are no blocks in the bin, find a block from a following bin and split them up, recursively 
  if(ptr == 0) {
    // If we have reached the last bin and haven't found any blocks, then there is no memory left
    if(block_size == MAX_BLOCK_SIZE) return 0;
    
    // Get a free block from the successor bin, if there are no larger blocks in any successive bin, return 0
    ptr = _kheap_get_free_block(block_size << 1);

    // If we have found a valid block, split it in half and push the second block into the bin 
    if(ptr != 0) _kheap_bin_insert(ptr+block_size, block_size);
  }

  return ptr;
}

kheap_ublock_t* _kheap_ublock_alloc() {
  size_t block_size = CONSTRAIN_TO_MIN_BLOCK_SIZE(ROUND_NEXT_POW2(sizeof(kheap_ublock_t)));
  vaddr_t free = _kheap_get_free_block(block_size);
  
  // If we haven't found a free block of the specified size, it is time to extend the heap region
  if(free == 0) {
    // Let's ask vmm for more heap
    vaddr_t vstart = vmm_km_heap_extend(PAGESIZE);
    _kheap_bin_insert(vstart, PAGESIZE);

    // Now we should be able to find a free block
    free = _kheap_get_free_block(block_size);
  }

  // We don't want to create a ublock for this block otherwise we will go into an infinite loop!
  // We know that this block must be freed whenever the ublock that it represents is freed
  return (kheap_ublock_t*)free;
}

void _kheap_ublock_free(kheap_ublock_t *ublock) {
  size_t block_size = CONSTRAIN_TO_MIN_BLOCK_SIZE(ROUND_NEXT_POW2(sizeof(kheap_ublock_t)));
  vaddr_t ptr = (vaddr_t)ublock;

  kassert(ptr != 0);

  // Place the ublock back in the free bin
  _kheap_bin_insert(ptr, block_size);
}

void _kheap_ublock_insert(vaddr_t used, size_t size) {
  kheap_ublock_t *ublock = _kheap_ublock_alloc();
  ublock->addr = used;
  ublock->size = size;
  ublock->next = ublock->prev = NULL;

  // If the list is empty
  if(root == NULL) {
    root = ublock;
    return;
  }

  // The used block list is sorted by ascending addresses
  kheap_ublock_t *next;
  for(next = root; next->next != NULL; next = next->next) {
    if(ublock->addr < next->addr) break;
  }
 
  // Insert the elements in the linked list
  ublock->next = next;
  ublock->prev = next->prev;
  if(next->prev != NULL) next->prev->next = ublock;
  else root = ublock;
  next->prev = ublock;

}

void _kheap_ublock_delete(kheap_ublock_t *ublock) {
  // Remove the element from the linked list
  if(ublock->prev != NULL) ublock->prev->next = ublock->next;
  else root = ublock->next;
  if(ublock->next != NULL) ublock->next->prev = ublock->prev; 

  // Free the ublock
  _kheap_ublock_free(ublock);
}

void _kheap_free_used_block(vaddr_t free) {
  // The used block list is sorted by ascending addresses
  kheap_ublock_t *next;
  for(next = root; next != NULL; next = next->next) {
    if(next->addr == free) break;
  }

  // Uh oh, we are trying to free memory that hasn't been allocated...or kheap fucked up somewhere hmmmm
  kassert(next != NULL);

  // We have found the used block! Free the ublock & the used block associated with it
  _kheap_bin_insert(next->addr, next->size);
  _kheap_ublock_delete(next);
}

void kheap_init() {
  // Initialize the kernel heap region
  vmm_km_heap_init();

  // Initially start with a heap of PAGESIZE bytes
  vaddr_t heap_start = vmm_km_heap_extend(PAGESIZE);
  _kheap_bin_insert(heap_start, PAGESIZE);
}

void* kheap_alloc(size_t size) {
  kassert(size != 0 && size <= MAX_BLOCK_SIZE);

  // Round size to the next power of 2 value and find a free block of that size
  size_t block_size = CONSTRAIN_TO_MIN_BLOCK_SIZE(ROUND_NEXT_POW2(size));
  vaddr_t free = _kheap_get_free_block(block_size);

  // If we haven't found a free block of the specified size, it is time to extend the heap region
  if(free == 0) {
    // Let's ask vmm for more heap
    vaddr_t vstart = vmm_km_heap_extend(block_size);

    // Since vmm rounds to PAGESIZE
    _kheap_bin_insert(vstart, ROUND_PAGE(block_size));

    // Now we should be able to find a free block
    free = _kheap_get_free_block(block_size);
  }

  // Now make an entry in the used block list
  _kheap_ublock_insert(free, block_size);

  return (void*)free;
}

void kheap_free(void *free) {
  // Make sure we aren't freeing an invalid pointer
  kassert(((vaddr_t)free) >= vmap_kernel()->heap_start || ((vaddr_t)free) < vmap_kernel()->heap_end);

 _kheap_free_used_block((vaddr_t)free); 

 // TODO: Shrink heap region?
}

