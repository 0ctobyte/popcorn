#include <kernel/page.h>
#include <kernel/kassert.h>
#include <lib/asm.h>
#include <string.h>

#define NUM_BINS                 (20)
#define MAX_NUM_CONTIGUOUS_PAGES (1l << NUM_BINS)

#define IS_POW2(n)                                   (((n) & ((n)-1)) == 0 && (n) != 0)
#define ROUND_DOWN_POW2(n)                           (_rbit(_rbit(n) & ~(_rbit(n) - 1l)))
#define ROUND_UP_POW2(n)                             IS_POW2(n) ? (n) : _rbit(1l << (_ctz(_rbit(n)) - 1))
#define GET_PAGE_INDEX(page)                         ((page) - page_array.pages)
#define GET_BUDDY_INDEX(page, num_pages)             (GET_PAGE_INDEX(page) ^ (num_pages))
#define GET_CONTIGUOUS_BUDDY(page, num_pages)        (&page_array.pages[GET_BUDDY_INDEX(page, num_pages)]);
#define GET_BIN_INDEX(num_pages)                     (_ctz(num_pages))
#define WHICH_BUDDY(page_index, bin)                 (page_index) & ~((1l << (bin)) - 1)

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
    spinlock_t lock;              // Multiple readers, single writer lock
    page_t *pages;                // Contiguous array of all pages
    page_t *page_bins[NUM_BINS];  // Buddy allocation bins
    size_t num_pages;             // Total # of pages
} page_array_t;

page_array_t page_array;

#ifdef DEBUG
#include <kernel/kstdio.h>
void page_dump_bins(void) {
    for (unsigned int bin = 0; bin < NUM_BINS; bin++) {
        kprintf("bin %u:", bin);
        for (page_t *buddy = page_array.page_bins[bin]; buddy != NULL; buddy = buddy->next_buddy) {
            kprintf(" %u", GET_PAGE_INDEX(buddy));
            if (buddy->next_buddy != NULL) kprintf(" ->");
        }
        kprintf("\n");
    }
}

void page_dump_page(page_t *page) {
    kprintf("Page Num: %u\n", GET_PAGE_INDEX(page));
    kprintf("\tpaddr          = %p\n", (GET_PAGE_INDEX(page) << PAGESHIFT) + MEMBASEADDR);
    kprintf("\tStatus         = R:%u,D:%u,A:%u\n", page->status.is_referenced, page->status.is_dirty, page->status.is_active);
    kprintf("\tnext_buddy     = %p\n", page->next_buddy);
    kprintf("\tnext_resident  = %p\n", page->next_resident);
};

void page_dump_pages(page_t *pages, size_t num_pages) {
    for (unsigned int i = 0; i < num_pages; i++) {
        page_dump_page(&pages[i]);
    }
}
#endif // DEBUG

page_t* _page_bin_pop(size_t num_pages) {
    unsigned long bin_index = GET_BIN_INDEX(num_pages);

    // Make sure we're not accessing past the number of bins
    if (bin_index >= NUM_BINS) return NULL;

    page_t *pages = page_array.page_bins[bin_index];
    if (pages == NULL) {
        // If the bin corresponding to num_pages doesn't have a free buddy check the next bin up
        // Split the contiguous pages into two buddies, place one in the current bin, return the other
        // Just return NULL if we couldn't find a contiguous range of pages of size num_pages
        pages = _page_bin_pop(num_pages << 1);
        if (pages == NULL) return NULL;
        page_t *buddy = GET_CONTIGUOUS_BUDDY(pages, num_pages);
        page_array.page_bins[bin_index] = buddy;
        buddy->next_buddy = NULL;
    } else {
        // Otherwise pop this buddy off of the bin
        page_array.page_bins[bin_index] = pages->next_buddy;
        pages->next_buddy = NULL;
    }

    return pages;
}

void _page_bin_push(page_t *pages, size_t num_pages) {
    unsigned long bin_index = GET_BIN_INDEX(num_pages);
    unsigned long page_index = GET_PAGE_INDEX(pages);

    // Don't do anything if bin_index is past the max number of bins we have
    if (bin_index < NUM_BINS) {
        // The buddy list is ordered by ascending page number, search for the position to insert these freed pages
        page_t *next, *prev = NULL, *prev_prev = NULL;
        for (next = page_array.page_bins[bin_index]; next != NULL; prev_prev = prev, prev = next, next = next->next_buddy) {
            if (page_index < GET_PAGE_INDEX(next)) break;
        }

        // Make sure we aren't putting back pages that have are already in the bin
        if (prev != NULL) kassert(prev != pages);

        // Check if the previous or next buddies are contiguous and can be merged
        unsigned long buddy_index = GET_BUDDY_INDEX(pages, num_pages);
        if (next != NULL && buddy_index == GET_PAGE_INDEX(next)) {
            // Next is a buddy that can be merged. Re-arrange the linked list to pull pages and next out of the list
            if (prev != NULL) prev->next_buddy = next->next_buddy;
            else page_array.page_bins[bin_index] = next->next_buddy;

            // Push this contiguous block of free pages in the next higher up bin
            pages->next_buddy = next->next_buddy = NULL;
            _page_bin_push(pages, num_pages << 1);
        } else if (prev != NULL && buddy_index == GET_PAGE_INDEX(prev)) {
            // The previous buddy can be merged. Re-arrange the linked list to pull these pages out of the list
            if (prev_prev != NULL) prev_prev->next_buddy = next;
            else page_array.page_bins[bin_index] = next;

            // Push this new contiguous block of free pages in the next higher up bin
            prev->next_buddy = pages->next_buddy = NULL;
            _page_bin_push(prev, num_pages << 1);
        } else {
            // No buddies, just insert the free pages into the buddy list
            pages->next_buddy = next;
            if (prev != NULL) prev->next_buddy = pages;
            else page_array.page_bins[bin_index] = pages;
        }
    }
}

void page_init(paddr_t page_array_addr) {
    page_array.lock = SPINLOCK_INIT;
    page_array.pages = (page_t*)page_array_addr;
    page_array.num_pages = MEMSIZE >> PAGESHIFT;

    // Clear the entire array
    memset((void*)page_array.pages, 0, page_array.num_pages * sizeof(page_t));

    // Split the pages up into groups of the largest powers of 2 possible and place them in the appropriate bins.
    // The bins hold the pointer to the first page in the buddy. Do this until we've accounted for all the pages
    size_t page_group_size = 0;
    for (unsigned long i = 0; i < page_array.num_pages; i += page_group_size) {
        page_group_size = ROUND_DOWN_POW2(page_array.num_pages - i);
        page_array.page_bins[GET_BIN_INDEX(page_group_size)] = &page_array.pages[i];
    }
}

page_t* page_alloc_contiguous(size_t num_pages) {
    kassert(num_pages <= page_array.num_pages && num_pages < MAX_NUM_CONTIGUOUS_PAGES);

    spinlock_irqacquire(&page_array.lock);

    // Make sure num_pages is a power of 2
    num_pages = ROUND_UP_POW2(num_pages);
    page_t *first_page = _page_bin_pop(num_pages);

    // If we found a valid block of pages, mark them as active
    if (first_page != NULL) {
        for (unsigned long i = 0; i < num_pages; i++) {
            first_page[i].status.is_active = 1;
        }
    }

    spinlock_irqrelease(&page_array.lock);
    return first_page;
}

void page_free_contiguous(page_t *pages, size_t num_pages) {
    kassert(pages != NULL && num_pages <= page_array.num_pages && num_pages < MAX_NUM_CONTIGUOUS_PAGES);

    num_pages = ROUND_UP_POW2(num_pages);

    // The page number must be a multiple of num_pages
    kassert((GET_PAGE_INDEX(pages) & (num_pages - 1)) == 0);

    spinlock_irqacquire(&page_array.lock);

    _page_bin_push(pages, num_pages);

    // Clear the active bit
    for (unsigned long i = 0; i < num_pages; i++) {
        pages[i].status.is_active = 0;
    }

    spinlock_irqrelease(&page_array.lock);
}

page_t* page_alloc(void) {
    return page_alloc_contiguous(1);
}

void page_free(page_t *page) {
    page_free_contiguous(page, 1);
}

paddr_t page_to_pa(page_t *page) {
    kassert(page != NULL);
    return (GET_PAGE_INDEX(page) << PAGESHIFT) + MEMBASEADDR;
}

page_t* page_from_pa(paddr_t pa) {
    return page_array.pages + ((pa - MEMBASEADDR) >> PAGESHIFT);
}

page_t* page_reserve_pa(paddr_t pa) {
    // Get the page to reserve
    page_t *page = page_from_pa(pa);

    unsigned long page_index = GET_PAGE_INDEX(page);

    // Search the bins backwards for the highest free buddy that this page may belong to
    for (int bin = 0; bin < NUM_BINS; bin++) {
        // This is the buddy we are looking for in this bin
        unsigned long buddy_index = WHICH_BUDDY(page_index, bin);

        page_t *buddy, *prev = NULL;
        for (buddy = page_array.page_bins[bin]; buddy != NULL; prev = buddy, buddy = buddy->next_buddy) {
            if (GET_PAGE_INDEX(buddy) == buddy_index) break;
        }

        // Found it. Remove the entire buddy from the bin and "free" the other pages in the buddy except for the page we want to reserve
        if (buddy != NULL) {
            page->status.is_active = 1;
            page->wired_count++;

            if (prev != NULL) prev->next_buddy = buddy->next_buddy;
            else page_array.page_bins[bin] = buddy->next_buddy;
            buddy->next_buddy == NULL;

            // Loop through the lower bins splitting the buddy in two, keeping the buddy that has the page we want to reserve and freeing the other buddy
            for (unsigned long i = bin; i > 0; i--) {
                unsigned long num_pages = 1l << (i - 1);
                page_t *buddy1 = &page_array.pages[WHICH_BUDDY(page_index, i)];
                page_t *buddy2 = &page_array.pages[WHICH_BUDDY(page_index, i - 1)];
                page_free_contiguous((buddy1 == buddy2) ? buddy1 + num_pages : buddy1, num_pages);
            }

            return page;
        }
    }

    // We should never get here otherwise we may be reserving a page that has already been allocated
    return NULL;
}

void page_relocate_array(vaddr_t va) {
    page_t *page_array_va = (page_t*)va;

    // Adjust very single buddy pointer in each bin
    for (unsigned int i = 0; i < NUM_BINS; i++) {
        if (page_array.page_bins[i] != NULL) page_array.page_bins[i] = (page_array.page_bins[i] - page_array.pages) + page_array_va;
        for (page_t *buddy = page_array.page_bins[i]; buddy != NULL; buddy = buddy->next_buddy) {
            if (buddy->next_buddy != NULL) buddy->next_buddy = (buddy->next_buddy - page_array.pages) + page_array_va;
        }
    }

    page_array.pages = page_array_va;
}
