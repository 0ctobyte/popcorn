#include <kernel/kassert.h>
#include <kernel/hash.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_page.h>

#define NUM_BINS                 (20)
#define MAX_NUM_CONTIGUOUS_PAGES (1l << (NUM_BINS - 1))

#define IS_POW2(n)                                   (((n) & ((n)-1)) == 0 && (n) != 0)
#define ROUND_DOWN_POW2(n)                           (arch_rbit(arch_rbit(n) & ~(arch_rbit(n) - 1l)))
#define ROUND_UP_POW2(n)                             IS_POW2(n) ? (n) : arch_rbit(1l << (arch_ctz(arch_rbit(n)) - 1))
#define GET_PAGE_INDEX(page)                         ((page) - vm_page_array.pages)
#define GET_BUDDY_INDEX(page, num_pages)             (GET_PAGE_INDEX(page) ^ (num_pages))
#define GET_CONTIGUOUS_BUDDY(page, num_pages)        (&vm_page_array.pages[GET_BUDDY_INDEX(page, num_pages)]);
#define GET_BIN_INDEX(num_pages)                     (arch_ctz(num_pages))
#define WHICH_BUDDY(vm_page_index, bin)              (vm_page_index) & ~((1l << (bin)) - 1)

#define VM_PAGE_HASH(object, offset)                 (hash64_fnv1a_pair((unsigned long)object, offset) % vm_page_hash_table.num_buckets)

extern paddr_t kernel_physical_start;
extern paddr_t kernel_physical_end;

// Using the buddy allocation algorithm to allow allocating contiguous sets of pages.
// NUM_BINS limits the max contiguous set that can be allocated. For NUM_BINS = 20 and page_size = 4KB that means 4GB worth of
// Physical pages can be allocated contiguously at one time. Each bin holds a linked list of buddies where each buddy has x number of pages
// determined by the bin#. For example, Bin0 has buddies of 1 page, Bin1 has 2 pages per buddy, Bin2 has 4 pages per buddy etc.
// The idea is that given the number of contiguous pages to be allocated, for example say 8 pages, we look at that bin to find
// the first free buddy and return the pointer to the first page in that buddy. Subsequent pages in that buddy can be accessesed
// as indices into an array since all the pages are arranged contigously in memory as referenced by *pagearray. If a bin does not
// contain any free buddies then we continue up to the next bin, find a free buddy, split it in two and place them in the lower bin
// as a list of two buddies of half the size. We continue that process as long as the higher-up bin has no buddies. It's the same idea
// when freeing pages, place the free pages in the appropriate bin ordering them by physical address. If two buddies are contiguous in terms
// of physical address then we can merge them into one buddy and place them in the higher up bin; this process continues recursively until
// it is no longer possible to do so. Note that this only allows allocating power-of-2 number of contiguous pages. Anything else is rounded
// up to the next power of 2 and we will have wasted pages.
typedef struct {
    spinlock_t lock[NUM_BINS];       // One lock per bin
    vm_page_t *pages;                // Contiguous array of all pages
    list_t ll_page_bins[NUM_BINS];   // Buddy allocation bins
    size_t num_pages;                // Total # of pages
} vm_page_array_t;

vm_page_array_t vm_page_array;

// The hash table keeps track of every single allocated page. Pages are looked up by vm_object_t pointer and offset in that object
typedef struct {
    spinlock_t *lock;   // Multiple readers, single writer lock per bucket
    list_t *ll_pages;   // Array of lists; each list contains a chain of vm_page_t's in the same hash bucket
    size_t num_buckets; // Number of buckets
} vm_page_hash_table_t;

vm_page_hash_table_t vm_page_hash_table;

list_compare_result_t _vm_page_compare(list_node_t *n1, list_node_t *n2) {
    unsigned long p1 = (uintptr_t)n1, p2 = (uintptr_t)n2;
    return (p1 < p2) ? LIST_COMPARE_LT : (p1 > p2) ? LIST_COMPARE_GT : LIST_COMPARE_EQ;
}

list_compare_result_t _vm_page_find(list_node_t *n1, list_node_t *n2) {
    vm_page_t *p1 = list_entry(n1, vm_page_t, ll_onode), *p2 = list_entry(n2, vm_page_t, ll_onode);
    return (p1->offset == p2->offset && p1->object == p2->object) ? LIST_COMPARE_EQ : LIST_COMPARE_LT;
}

vm_page_t* _vm_page_bin_pop(size_t num_pages) {
    unsigned long bin_index = GET_BIN_INDEX(num_pages);

    // Make sure we're not accessing past the number of bins
    if (bin_index >= NUM_BINS) return NULL;

    vm_page_t *pages = NULL;

    spinlock_acquire(&vm_page_array.lock[bin_index]);

    if (list_is_empty(&vm_page_array.ll_page_bins[bin_index])) {
        // If the bin corresponding to num_pages doesn't have a free buddy check the next bin up
        // Split the contiguous pages into two buddies, place one in the current bin, return the other
        // Just return NULL if we couldn't find a contiguous range of pages of size num_pages
        spinlock_release(&vm_page_array.lock[bin_index]);
        pages = _vm_page_bin_pop(num_pages << 1);
        if (pages == NULL) return NULL;

        vm_page_t *buddy = GET_CONTIGUOUS_BUDDY(pages, num_pages);

        spinlock_acquire(&vm_page_array.lock[bin_index]);
        kassert(list_push(&vm_page_array.ll_page_bins[bin_index], &buddy->ll_rnode));
    } else {
        // Otherwise pop this buddy off of the bin
        list_node_t *node = NULL;
        kassert(list_pop(&vm_page_array.ll_page_bins[bin_index], node));
        pages = list_entry(node, vm_page_t, ll_rnode);
    }

    spinlock_release(&vm_page_array.lock[bin_index]);

    return pages;
}

void _vm_page_bin_push(vm_page_t *pages, size_t num_pages) {
    unsigned long bin_index = GET_BIN_INDEX(num_pages);

    // Don't do anything if bin_index is past the max number of bins we have
    if (bin_index < NUM_BINS) {
        spinlock_acquire(&vm_page_array.lock[bin_index]);

        // The buddy list is ordered by ascending page number, search for the position to insert these freed pages
        kassert(list_insert(&vm_page_array.ll_page_bins[bin_index], _vm_page_compare, &pages->ll_rnode, LIST_ORDER_ASCENDING));

        vm_page_t *next = list_entry(list_next(&pages->ll_rnode), vm_page_t, ll_rnode);
        vm_page_t *prev = list_entry(list_prev(&pages->ll_rnode), vm_page_t, ll_rnode);

        // Check if the previous or next buddies are contiguous and can be merged
        unsigned long buddy_index = GET_BUDDY_INDEX(pages, num_pages);
        bool merge_buddy = false;

        if (next != NULL && buddy_index == GET_PAGE_INDEX(next)) {
            // Next is a buddy that can be merged. Re-arrange the linked list to pull pages and next out of the list
            kassert(list_remove(&vm_page_array.ll_page_bins[bin_index], &next->ll_rnode));
            kassert(list_remove(&vm_page_array.ll_page_bins[bin_index], &pages->ll_rnode));

            merge_buddy = true;
        } else if (prev != NULL && buddy_index == GET_PAGE_INDEX(prev)) {
            // The previous buddy can be merged. Re-arrange the linked list to pull these pages out of the list
            kassert(list_remove(&vm_page_array.ll_page_bins[bin_index], &prev->ll_rnode));
            kassert(list_remove(&vm_page_array.ll_page_bins[bin_index], &pages->ll_rnode));

            merge_buddy = true;
            pages = prev;
        }

        spinlock_release(&vm_page_array.lock[bin_index]);

        // Push this contiguous block of free pages in the next higher up bin
        if (merge_buddy) _vm_page_bin_push(pages, num_pages << 1);
    }
}

void _vm_page_insert(vm_page_t *pages, size_t num_pages, vm_object_t *object, vm_offset_t starting_offset) {
    // Add each page in the list to the object and update the pages object and offset fields
    // Also add the page to the hash table
    for (unsigned int p = 0; p < num_pages; p++) {
        vm_offset_t offset = starting_offset + (p << PAGESHIFT);
        if (offset >= object->size) object->size += (offset - object->size) + PAGESIZE;

        pages[p].object = object;
        pages[p].offset = offset;

        kassert(list_insert_last(&object->ll_resident, &pages[p].ll_rnode));

        unsigned long hash_bkt = VM_PAGE_HASH(object, offset);

        spinlock_write_acquire(&vm_page_hash_table.lock[hash_bkt]);
        kassert(list_insert_last(&vm_page_hash_table.ll_pages[hash_bkt], &pages[p].ll_onode));
        spinlock_write_release(&vm_page_hash_table.lock[hash_bkt]);
    }
}

void _vm_page_remove(vm_page_t *pages, size_t num_pages) {
    // Remove each page from the list and the hash table
    for (unsigned int p = 0; p < num_pages; p++) {
        vm_object_t *object = pages[p].object;
        vm_offset_t offset = pages[p].offset;

        unsigned long hash_bkt = VM_PAGE_HASH(object, offset);

        spinlock_write_acquire(&vm_page_hash_table.lock[hash_bkt]);
        kassert(list_remove(&vm_page_hash_table.ll_pages[hash_bkt], &pages[p].ll_onode));
        spinlock_write_release(&vm_page_hash_table.lock[hash_bkt]);

        kassert(list_remove(&object->ll_resident, &pages[p].ll_rnode));

        pages[p].object = NULL;
        pages[p].offset = 0;
    }
}

void vm_page_init(void) {
    // Allocate space for the vm_page_array
    size_t vm_page_array_size = ROUND_PAGE_UP((MEMSIZE >> PAGESHIFT) * sizeof(vm_page_t));
    vaddr_t vm_page_array_addr = (vaddr_t)pmap_steal_memory(vm_page_array_size, NULL, NULL);

    vm_page_array.pages = (vm_page_t*)vm_page_array_addr;
    vm_page_array.num_pages = MEMSIZE >> PAGESHIFT;

    for (unsigned long i = 0; i < NUM_BINS; i++) {
        vm_page_array.lock[i] = SPINLOCK_INIT;
        vm_page_array.ll_page_bins[i] = LIST_INITIALIZER;
    }

    // Clear the entire array
    arch_fast_zero(vm_page_array.pages, vm_page_array.num_pages * sizeof(vm_page_t));

    // Split the pages up into groups of the largest powers of 2 possible and place them in the appropriate bins.
    // The bins hold the pointer to the first page in the buddy. Do this until we've accounted for all the pages
    size_t vm_page_group_size = 0;
    for (unsigned long i = 0; i < vm_page_array.num_pages; i += vm_page_group_size) {
        vm_page_group_size = ROUND_DOWN_POW2(vm_page_array.num_pages - i);
        vm_page_group_size = vm_page_group_size > MAX_NUM_CONTIGUOUS_PAGES ? MAX_NUM_CONTIGUOUS_PAGES : vm_page_group_size;
        kassert(list_push(&vm_page_array.ll_page_bins[GET_BIN_INDEX(vm_page_group_size)], &vm_page_array.pages[i].ll_rnode));
    }

    // Allocate space for the vm_page_t hash table. No kmem at this point so use pmap_steal_memory
    // Hash table size is 1.5 times the number of pages
    vm_page_hash_table.num_buckets = vm_page_array.num_pages + (vm_page_array.num_pages >> 1);
    size_t size = vm_page_hash_table.num_buckets * sizeof(list_t);
    size_t lock_size = vm_page_hash_table.num_buckets * sizeof(spinlock_t);

    vm_page_hash_table.lock = (spinlock_t*)pmap_steal_memory(lock_size, NULL, NULL);
    vm_page_hash_table.ll_pages = (list_t*)pmap_steal_memory(size, NULL, NULL);
    arch_fast_zero(vm_page_hash_table.ll_pages, size);
    arch_fast_zero(vm_page_hash_table.lock, lock_size);

    // Reserve the physical pages used by the kernel
    for (size_t pa = kernel_physical_start; pa < kernel_physical_end; pa += PAGESIZE) {
        vm_page_reserve_pa(pa);
    }

    // Add all the pages allocated for the kernel up to this point to the kernel object
    size_t num_pages = (kernel_physical_end - kernel_physical_start) >> PAGESHIFT;
    vm_page_t *pages = vm_page_from_pa(kernel_physical_start);
    _vm_page_insert(pages, num_pages, &kernel_object, 0);
}

#if DEBUG
void vm_page_hash_table_dump(void) {
    extern void kprintf(const char *s, ...);
    for (unsigned long i = 0; i < vm_page_hash_table.num_buckets; i++) {
        spinlock_read_acquire(&vm_page_hash_table.lock[i]);

        if (list_is_empty(&vm_page_hash_table.ll_pages[i])) {
            spinlock_read_release(&vm_page_hash_table.lock[i]);
            continue;
        }

        kprintf("%u", i);

        vm_page_t *page;
        list_for_each_entry(&vm_page_hash_table.ll_pages[i], page, ll_onode) {
            kprintf(" -> %p", vm_page_to_pa(page));
        }

        spinlock_read_release(&vm_page_hash_table.lock[i]);

        kprintf("\n");
    }
}
#endif // DEBUG

vm_page_t* vm_page_lookup(vm_object_t *object, vm_offset_t offset) {
    kassert(object != NULL);

    offset = ROUND_PAGE_DOWN(offset);
    unsigned long hash_bkt = VM_PAGE_HASH(object, offset);

    spinlock_read_acquire(&vm_page_hash_table.lock[hash_bkt]);

    vm_page_t p = { .status = {0}, .ll_onode = LIST_NODE_INITIALIZER, .ll_rnode = LIST_NODE_INITIALIZER, .object = object, .offset = offset };
    list_node_t *node = list_search(&vm_page_hash_table.ll_pages[hash_bkt], _vm_page_find, &p.ll_onode);

    spinlock_read_release(&vm_page_hash_table.lock[hash_bkt]);

    return list_entry(node, vm_page_t, ll_onode);
}

vm_page_t* vm_page_alloc_contiguous(size_t num_pages, vm_object_t *object, vm_offset_t offset) {
    kassert(num_pages <= vm_page_array.num_pages && num_pages <= MAX_NUM_CONTIGUOUS_PAGES);

    // Make sure num_pages is a power of 2
    num_pages = ROUND_UP_POW2(num_pages);
    vm_page_t *first_page = _vm_page_bin_pop(num_pages);

    // If we found a valid block of pages, mark them as active
    if (first_page != NULL) {
        for (unsigned long i = 0; i < num_pages; i++) {
            first_page[i].status.is_active = 1;
        }
    }

    // If an object is specified, add the page(s) to that object
    if (first_page != NULL && object != NULL) {
        spinlock_write_acquire(&object->lock);
        _vm_page_insert(first_page, num_pages, object, offset);
        spinlock_write_release(&object->lock);
    }

    return first_page;
}

void vm_page_free_contiguous(vm_page_t *pages, size_t num_pages) {
    kassert(pages != NULL && num_pages <= vm_page_array.num_pages && num_pages <= MAX_NUM_CONTIGUOUS_PAGES);

    num_pages = ROUND_UP_POW2(num_pages);

    // The page number must be a multiple of num_pages
    kassert((GET_PAGE_INDEX(pages) & (num_pages - 1)) == 0);

    // Assuming all pages belong to the same object
    vm_object_t *object = pages[0].object;
    if (object != NULL) {
        spinlock_write_acquire(&object->lock);
        _vm_page_remove(pages, num_pages);
        spinlock_write_release(&object->lock);
    }

    _vm_page_bin_push(pages, num_pages);

    // Clear the active bit
    for (unsigned long i = 0; i < num_pages; i++) {
        pages[i].status.is_active = 0;
    }
}

vm_page_t* vm_page_alloc(vm_object_t *object, vm_offset_t offset) {
    return vm_page_alloc_contiguous(1, object, offset);
}

void vm_page_free(vm_page_t *page) {
    vm_page_free_contiguous(page, 1);
}

void vm_page_wire(vm_page_t *page) {
    if (page->object != NULL) spinlock_write_acquire(&page->object->lock);
    page->status.wired_count++;
    if (page->object != NULL) spinlock_write_release(&page->object->lock);
}

void vm_page_unwire(vm_page_t *page) {
    if (page->object != NULL) spinlock_write_acquire(&page->object->lock);
    page->status.wired_count--;
    if (page->object != NULL) spinlock_write_release(&page->object->lock);
}

paddr_t vm_page_to_pa(vm_page_t *page) {
    kassert(page != NULL);
    return (GET_PAGE_INDEX(page) << PAGESHIFT) + MEMBASEADDR;
}

vm_page_t* vm_page_from_pa(paddr_t pa) {
    return vm_page_array.pages + ((pa - MEMBASEADDR) >> PAGESHIFT);
}

vm_page_t* vm_page_reserve_pa(paddr_t pa) {
    // Get the page to reserve
    vm_page_t *page = vm_page_from_pa(pa);

    unsigned long vm_page_index = GET_PAGE_INDEX(page);

    // Search the bins backwards for the highest free buddy that this page may belong to
    for (int bin = 0; bin < NUM_BINS; bin++) {
        // This is the buddy we are looking for in this bin
        unsigned long buddy_index = WHICH_BUDDY(vm_page_index, bin);
        vm_page_t *buddy = list_entry(list_search(&vm_page_array.ll_page_bins[bin], _vm_page_compare, &vm_page_array.pages[buddy_index].ll_rnode), vm_page_t, ll_rnode);

        // Found it. Remove the entire buddy from the bin and "free" the other pages in the buddy except for the page we want to reserve
        if (buddy != NULL) {
            page->status.is_active = 1;
            page->status.wired_count++;

            kassert(list_remove(&vm_page_array.ll_page_bins[bin], &buddy->ll_rnode));

            // Loop through the lower bins splitting the buddy in two, keeping the buddy that has the page we want to reserve and freeing the other buddy
            for (unsigned long i = bin; i > 0; i--) {
                unsigned long num_pages = 1l << (i - 1);
                vm_page_t *buddy1 = &vm_page_array.pages[WHICH_BUDDY(vm_page_index, i)];
                vm_page_t *buddy2 = &vm_page_array.pages[WHICH_BUDDY(vm_page_index, i - 1)];
                vm_page_free_contiguous((buddy1 == buddy2) ? buddy1 + num_pages : buddy1, num_pages);
            }

            return page;
        }
    }

    // We should never get here otherwise we may be reserving a page that has already been allocated
    return NULL;
}
