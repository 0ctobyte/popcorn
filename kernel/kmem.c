#include <kernel/kassert.h>
#include <kernel/spinlock.h>
#include <kernel/kstdio.h>
#include <kernel/list.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/slab.h>
#include <kernel/vm/vm_km.h>
#include <kernel/kmem.h>

#define INITIAL_SLAB_BUF_COUNT         (64)

// Minimum block size of 32B and maximum block size of 64KB
#define NUM_BINS                       (12)
#define MIN_BLOCK_SIZE                 (0x20)
#define MAX_BLOCK_SIZE                 (1ul << ((NUM_BINS-1) + arch_ctz(MIN_BLOCK_SIZE)))
#define BIN_TO_BLOCK_SIZE(x)           (1ul << ((x) + arch_ctz(MIN_BLOCK_SIZE)))

#define IS_POW2(n)                     (((n) & ((n)-1)) == 0 && (n) != 0)
#define ROUND_DOWN_POW2(n)             (arch_rbit(arch_rbit(n) & ~(arch_rbit(n) - 1ul)))
#define ROUND_UP_POW2(n)               (IS_POW2(n) ? (n) : arch_rbit(1ul << (arch_ctz(arch_rbit(n)) - 1)))
#define CONSTRAIN_TO_MIN_BLOCK_SIZE(n) (((n) < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : (n))
#define GET_BIN_INDEX(n)               (arch_ctz(n) - arch_ctz(MIN_BLOCK_SIZE))

typedef struct {
    slab_t slab;                // Slab struct for this bin
    list_node_t ll_node;        // List linkage for lru
    size_t total_slab_size;     // Combined size of all slab buffers in this bin
    unsigned long total_allocs; // Total number of allocations for this bin
    unsigned long total_frees;  // Total number of frees for this bin
} kmem_bin_t;

typedef struct {
    kmem_bin_t bins[NUM_BINS];  // Array of bins, each bin holds a slab of power-of-2 sized blocks
    list_t ll_lru;              // List of all bins ordered by least recently allocated
    slab_t slab_buf_slab;       // Slab to allocate slab_buf_t's from
    size_t total_slab_size;     // Combined size of all slabs from each bin
    unsigned long total_allocs; // Total number of allocations
    unsigned long total_frees;  // Total number of frees
    spinlock_t lock;            // Lock
} kmem_t;

kmem_t kmem;

void _kmem_grow(unsigned int bin) {
    size_t block_size = BIN_TO_BLOCK_SIZE(bin);

    kassert(kmem.total_allocs != 0 && kmem.bins[bin].total_allocs != 0);

    // Calculate the percentage of total allocations that come from this bin
    // Use that ratio to calculate how much to increase the slab for this bin by (rounded up to the nearest page)
    long alloc_pct = (kmem.bins[bin].total_allocs * 100) / kmem.total_allocs;
    long increase = ROUND_PAGE_UP(((kmem.bins[bin].total_slab_size) * alloc_pct) / 100);
    increase = (increase < block_size) ? block_size : increase;

    // Try and grab pages from other least recently used bins if possible. Only check half the bins in the LRU list
    // This bin should be at the end of this list by now
    kmem_bin_t *lru = list_entry(list_first(&kmem.ll_lru), kmem_bin_t, ll_node);
    for (unsigned int i = 0; i < (NUM_BINS / 2) && !list_end(lru); i++) {
        slab_buf_t *slab_buf = slab_shrink(&lru->slab);

        if (slab_buf != NULL) {
            size_t slab_buf_size = SLAB_BUF_SIZE(&lru->slab, slab_buf);

            // This slab buffer is too small for an allocation of one block, put it back from where it came
            if (slab_buf_size < block_size) {
                slab_grow(&lru->slab, slab_buf, slab_buf->buf, slab_buf_size);
                continue;
            }

            slab_grow(&kmem.bins[bin].slab, slab_buf, slab_buf->buf, slab_buf_size);

            kmem.bins[bin].total_slab_size += slab_buf_size;
            lru->total_slab_size -= slab_buf_size;

            increase -= slab_buf_size;
            if (increase <= 0) break;
        }

        lru = list_entry(list_next(&lru->ll_node), kmem_bin_t, ll_node);
    }

    // If we can't satisfy memory by stealing slab buffers from least recently used bins, then ask the VM system for more pages
    if (increase > 0) {
        vaddr_t va = vm_km_alloc(increase, VM_KM_FLAGS_WIRED);

        kmem.bins[bin].total_slab_size += increase;
        kmem.total_slab_size += increase;

        // Allocate a slab_buf_t for this new slab buffer; allocate more memory from VM system if we are out of the slab_buf_t slab
        slab_buf_t *slab_buf = (slab_buf_t*)slab_alloc(&kmem.slab_buf_slab);
        if (slab_buf == NULL) {
            size_t size = ROUND_PAGE_UP(sizeof(slab_buf_t)*INITIAL_SLAB_BUF_COUNT);
            vaddr_t sa = vm_km_alloc(size, VM_KM_FLAGS_WIRED);
            slab_grow(&kmem.slab_buf_slab, NULL, (void*)sa, size);

            slab_buf = (slab_buf_t*)slab_alloc(&kmem.slab_buf_slab);
        }

        slab_grow(&kmem.bins[bin].slab, slab_buf, (void*)va, increase);
    }
}

void kmem_init(void) {
    arch_fast_zero((unsigned long)&kmem, sizeof(kmem_t));

    // Grab some space for the slab_buf slab
    size_t size = ROUND_PAGE_UP(sizeof(slab_buf_t)*INITIAL_SLAB_BUF_COUNT);
    vaddr_t va = vm_km_alloc(size, VM_KM_FLAGS_WIRED);
    slab_init(&kmem.slab_buf_slab, NULL, (void*)va, size, sizeof(slab_buf_t));

    // Get the total size of memory needed for all bins (estimate at least one page for each bin)
    for (unsigned int i = 0; i < NUM_BINS; i++) {
        size_t size = ROUND_PAGE_UP(BIN_TO_BLOCK_SIZE(i));
        kmem.total_slab_size += size;
        kmem.bins[i].total_slab_size = size;
        list_push(&kmem.ll_lru, &kmem.bins[i].ll_node);
    }

    va = vm_km_alloc(kmem.total_slab_size, VM_KM_FLAGS_WIRED);

    // Divvy up allocated memory to each bin
    for (unsigned int i = 0; i < NUM_BINS; i++) {
        slab_buf_t *slab_buf = (slab_buf_t*)slab_alloc(&kmem.slab_buf_slab);
        slab_init(&kmem.bins[i].slab, slab_buf, (void*)va, kmem.bins[i].total_slab_size, BIN_TO_BLOCK_SIZE(i));
        va += kmem.bins[i].total_slab_size;
    }
}

void* kmem_alloc(size_t size) {
    if (size > MAX_BLOCK_SIZE) return NULL;

    // Round size to a power-of-2 size
    size = CONSTRAIN_TO_MIN_BLOCK_SIZE(ROUND_UP_POW2(size));
    unsigned int bin = GET_BIN_INDEX(size);

    spinlock_acquire(&kmem.lock);

    // Update book-keeping
    list_remove(&kmem.ll_lru, &kmem.bins[bin].ll_node);
    list_insert_last(&kmem.ll_lru, &kmem.bins[bin].ll_node);
    kmem.bins[bin].total_allocs++;
    kmem.total_allocs++;

    void *block = slab_alloc(&kmem.bins[bin].slab);

    if (block == NULL) {
        // Grow the bin if we're out of slab memory
        _kmem_grow(bin);
        block = slab_alloc(&kmem.bins[bin].slab);
        kassert(block != NULL);
    }

    spinlock_release(&kmem.lock);

    return block;
}

void* kmem_zalloc(size_t size) {
    void *mem = kmem_alloc(size);
    if (mem != NULL) arch_fast_zero((uintptr_t)mem, size);
    return mem;
}

void kmem_free(void *mem, size_t size) {
    kassert(size <= MAX_BLOCK_SIZE);

    // Round size to a power-of-2 size
    size = CONSTRAIN_TO_MIN_BLOCK_SIZE(ROUND_UP_POW2(size));
    unsigned int bin = GET_BIN_INDEX(size);

    spinlock_acquire(&kmem.lock);

    // Update book-keeping
    kmem.bins[bin].total_frees++;
    kmem.total_frees++;

    slab_free(&kmem.bins[bin].slab, mem);

    spinlock_release(&kmem.lock);
}

void kmem_stats(void) {
    unsigned long total_alloc_size = 0;

    kprintf("KMEM STATS\n");
    kprintf("----------\n");
    kprintf("\tTotal\t\tTotal\tCurrent\t\t\t\t\t\t\tTotal\tTotal\t\tTotal\n");
    kprintf("\tAllocations\tFrees\tAllocations\tAllocated\tSlab\t\tUsed\tUsed %%\tAllocated %%\tSlab %%\n");

    for (unsigned int i = 0; i < NUM_BINS; i++) {
        unsigned long current_allocs = kmem.bins[i].total_allocs - kmem.bins[i].total_frees;
        unsigned long alloc_size = current_allocs * BIN_TO_BLOCK_SIZE(i);
        unsigned long used_pct = (alloc_size * 100) / kmem.bins[i].total_slab_size;
        unsigned long total_used_pct = (alloc_size * 100) / kmem.total_slab_size;
        unsigned long total_alloc_pct = (current_allocs * 100) / kmem.total_allocs;
        unsigned long total_slab_pct = (kmem.bins[i].total_slab_size * 100) / kmem.total_slab_size;

        total_alloc_size += alloc_size;

        kprintf("%5uB:\t%u\t\t%u\t%u\t\t%uB\t\t%uB\t\t%u%%\t%u%%\t%u%%\t\t%u%%\n", BIN_TO_BLOCK_SIZE(i), kmem.bins[i].total_allocs, kmem.bins[i].total_frees, current_allocs,
            alloc_size, kmem.bins[i].total_slab_size, used_pct, total_used_pct, total_alloc_pct, total_slab_pct);
    }

    unsigned long total_current_allocs = kmem.total_allocs - kmem.total_frees;
    kprintf("------\n");
    kprintf("Total:\t%u\t\t%u\t%u\t\t%uB\t\t%uB\n", kmem.total_allocs, kmem.total_frees, total_current_allocs, total_alloc_size, kmem.total_slab_size);
}
