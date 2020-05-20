#include <kernel/pmm.h>
#include <kernel/kassert.h>
#include <kernel/spinlock.h>
#include <lib/bitmap.h>
#include <lib/asm.h>
#include <limits.h>

// The number of bits in each bitmap
#define BITS (64)

// Set a bit and clear a bit in the bitmap
#define SET_PAGE(bitmap, rel_page_num)   (bitmap) |= (1 << (rel_page_num))
#define CLEAR_PAGE(bitmap, rel_page_num) (bitmap) &= ~(1 << (rel_page_num))

// Multiply pagemap array index by BITS and subtract value from absolute page number to get relative page number within a bitmap.
#define ABS_TO_REL_PAGE_NUM(abs_page_num, elem_num)   ((abs_page_num) - ((elem_num) * BITS))
#define ABS_PAGE_NUM_TO_PADDR(abs_page_num)           ((abs_page_num) * PMM_PAGE_SIZE + pagestack.mem_base_addr)
#define REL_TO_ABS_PAGE_NUM(rel_page_num, elem_num)   (((elem_num) * BITS) + (rel_page_num))
#define REL_PAGE_NUM_TO_PADDR(rel_page_num, elem_num) (ABS_PAGE_NUM_TO_PADDR((REL_TO_ABS_PAGE_NUM(rel_page_num, elem_num))))

// Divide by #bits per pagemap. This gives the index into the pagemaps array
#define GET_PAGEMAP_ARRAY_INDEX(abs_page_num) ((abs_page_num) >> (_ctz(BITS)))

#define IS_WITHIN_MEM_BOUNDS(addr) ((((addr) >= pagestack.mem_base_addr) && ((addr) < (pagestack.mem_base_addr + pagestack.mem_size))))
#define IS_PAGE_ALIGNED(addr)      (((addr) & (PMM_PAGE_SIZE - 1)) == 0)
#define ROUND_PAGE_DOWN(addr)      ((addr) & ~(PMM_PAGE_SIZE - 1))
#define ATOP(addr)                 (((addr) - pagestack.mem_base_addr) >> PMM_PAGE_SHIFT)
#define PTOA(addr)                 (((addr) << PMM_PAGE_SHIFT) + pagestack.mem_base_addr)

/*
 * So how does this physical memory allocator work?
 * The basic idea is that main memory is split into fixed sized partitions
 * called pages. This allocator then simply hands out such pages and frees
 * them when asked to do so. Also, the allocator should be able to allocate
 * a contiguous, potentially aligned by a multiple of PAGESIZE, range of free
 * pages.
 *
 * One of the popular ways to do this is to use a bitmap, an array of bits,
 * where each bit represents a page in memory. In terms of speed, this method
 * is not very efficient since operations such as allocating a page, freeing
 * a page and allocating a contiguous range of pages would all take O(n) time
 * (n is the number of pages in memory).
 *
 * Another popular method is to use a stack of free pages. The stack will hold
 * the physical addresses to each free page. Thus, when allocating a page,
 * the first page on the stack is simply popped off. Similarly, when freeing a
 * page, the address of that page is pushed on to the stack. With a stack,
 * allocating and freeing a page takes constant time. This is great! However,
 * there are some problems.
 *
 * Specifically, memory use and allocating a contiguous range of free pages.
 * Assuming we are on a 32-bit system with the maximum 4 GB of RAM and a
 * page size of 4 KB, a bitmap would take about 128 kb of memory. A stack on the
 * other hand, where a memory address takes about 4 bytes, would take 4 Mb of
 * memory. Not only that but trying to allocate a continuous range of free
 * pages using the stack method is non-trivial.
 *
 * So what now? Well, why not combine the best of stacks and bitmaps!!one!1!?
 * So here's the idea on how this physical memory allocator works. We will have
 * a struct which I call a 'pagemap'. This pagemap will have a bitmap with #BITS
 * bits and a pointer to the next (in the stack) pagemap. This pagemap will be
 * 8 bytes in size. There will be an array of these pagemaps, the size of the
 * array depending on the size of the memory and the page size. For example, on
 * a 4 GB system with a page size of 4 kb, there will be 32768 of these
 * pagemaps which means 256 kb will be used for this array. Now, each bit in
 * the bitmaps of each pagemap represents one page. Okay, so initially each of
 * these pagemaps will be linked via the next pointer (the last pagemap in the
 * array of course will have a NULL next pointer). This linked array is our
 * stack, the top of the stack is the first element in the array INITIALLY.
 * This will change as pages are allocated and freed. Now, when we want to
 * allocate a page we simple look at the first pagemap in the stack, find
 * a free bit in the #BITS-bit bitmap and calculate the address using the
 * position of the pagemap in the array (element number) and the position of
 * the free bit in the bitmap. For example, the 133rd page would be represented
 * by the 5th bit (133-#BITS*floor(133/#BITS))in the 4th pagemap
 * (floor(133/#BITS)) in the array. If the bitmap has been fully set after
 * allocating the page then we simply pop it off the stack. When freeing a page,
 * we use the page address to calculate the position of the pagemap in the array
 * and position of the bit in the bitmap of that pagemap and then unset it. If
 * that bitmap was previously fully allocated then we simply push it on the
 * pagemap stack.
 *
 * Thus, using this method, which I call the 'bitstack' method :),
 * allocating and freeing a single page takes O(1) time. So what about
 * contiguous page allocation and memory use? Well I mentioned above that
 * this method takes 256 kb of memory max on a 32 bit system with a page size
 * of 4 kb (standard). That's just double the memory use of a bitmap (128 kb)
 * and compared to the stack method (4 Mb), 256 kb looks great! Now for
 * contiguous page allocation. Contiguous page allocation is still not
 * very efficient using this method as it will take O(n) time where n is
 * the number of PAGEMAPS in the array. The pmm_alloc_contiguous function
 * below uses a function, bitmap_find_contiguous_zeros, which employs a for loop.
 * However, this for loop NEVER loops more than #BITS times MAX. And it would
 * rarely ever loop #BITS times (in most cases it will loop less than half that
 * number). Thus, O(n) is a valid estimate of pmm_alloc_contiguous (in case
 * anyone was wondering).
 *
 * However, using the bitmap method, contiguous page allocation takes O(m) time
 * where m is the number of PAGES. The number of pagemaps in the array will
 * always be less than the number of pages in memory. Thus, the bitstack
 * method is able to allocate contiguous pages faster than using the bitmap
 * method. Contiguous page allocation using the stack method, disregarding the
 * complexity of code and logic required to implement such a function, would
 * take O(m) time as well.
 *
 * Thus, with the bitstack method, we get similar performance for allocating
 * and freeing a page as we do with the stack method, memory efficiency
 * comparable to the bitmap method and faster contiguous page allocation
 * than both the bitmap and stack method! However, the only downside of the
 * bitstack method is that only #BITS contiguous pages can be allocated at once
 * (since the pagemaps are in a contiguous array, there could be ways to
 * circumvent this limit).
 * For the purposes of this kernel, this limit on the contiguous pages
 * shouldn't be much of a problem.
 */

typedef struct pagemap_t {
    unsigned long bitmap;     // Each bit represents one page
    struct pagemap_t *next;   // Pointer to next pagemap in the stack
} pagemap_t;

typedef struct {
    spinlock_t lock;
    pagemap_t *pagemaps;     // Array of pagemap_t
    pagemap_t *top;	         // Top of stack
    size_t size;             // # of pagemap_t in the pagemap_t array
    uintptr_t mem_base_addr; // Base address of memory
    size_t mem_size;         // Size of memory in bytes
} pagestack_t;

static pagestack_t pagestack = {SPINLOCK_INIT, 0, 0, 0};

extern void* pmap_steal_memory(size_t size);

// Pushes the given pagemap on the stack
void _pmm_push(pagestack_t *pstack, pagemap_t *pagemap) {
    pagemap->next = pstack->top;
    pstack->top = pagemap;
}

// Pops the pagemap off the stack
pagemap_t* _pmm_pop(pagestack_t *pstack) {
    if(pstack->top == NULL) return NULL;
    pagemap_t *next = pstack->top->next;
    pagemap_t *popped = pstack->top;
    pstack->top->next = NULL;
    pstack->top = next;
    return popped;
}

size_t pmm_get_size_requirement() {
    return ((MEMSIZE >> (_ctz(PMM_PAGE_SIZE))) >> (_ctz(BITS))) * sizeof(pagemap_t);
}

void pmm_init(vaddr_t va) {
    // Allocate memory for the pagemaps
    // pmap_steal_memory will only work if pmap_init has been called
    pagestack.mem_base_addr = MEMBASEADDR;
    pagestack.mem_size = MEMSIZE;
    pagestack.size = ((pagestack.mem_size >> (_ctz(PMM_PAGE_SIZE))) >> (_ctz(BITS)));
    pagestack.pagemaps = (pagemap_t*)va;

    // Link the pagemaps together
    pagestack.top = pagestack.pagemaps;
    pagemap_t *p = pagestack.pagemaps;
    for(unsigned long i = 0; i < pagestack.size; ++i) {
        // Make sure to clear the bitmaps
        p[i].bitmap = 0;
        p[i].next = ((i+1) < pagestack.size) ? &p[i+1] : NULL;
    }
}

bool pmm_alloc(paddr_t *addr) {
    spinlock_irqacquire(&pagestack.lock);
    kassert(pagestack.top != NULL || addr == NULL);

    pagemap_t *top = pagestack.top;

    // Find first free page in pagemap and set the bit in the bitmap
    // Calculate address using location of pagemap in the pagestacks.pagemaps array and the bit number in the pagemap's bitmap
    unsigned long page = bitmap_find_contiguous_zeros(top->bitmap, 1);
    SET_PAGE(top->bitmap, page);
    *addr = REL_PAGE_NUM_TO_PADDR(page, (top - pagestack.pagemaps));

    // If pagemap is fully allocated, pop it off the stack
    if(top->bitmap == ULONG_MAX) _pmm_pop(&pagestack);
    spinlock_irqrelease(&pagestack.lock);
    return true;
}

void pmm_free(paddr_t addr) {
    // Check to make sure addr is page_aligned and within bounds
    kassert(IS_PAGE_ALIGNED(addr) && IS_WITHIN_MEM_BOUNDS(addr));

    // Calculate the absolute page number
    // Get the index into the pagemaps array
    // Get the page number relative to the bitmap in the pagemap
    unsigned long page_num = ATOP((addr));
    unsigned long elem_num = GET_PAGEMAP_ARRAY_INDEX(page_num);
    unsigned long rel_page_num = ABS_TO_REL_PAGE_NUM(page_num, elem_num);

    spinlock_irqacquire(&pagestack.lock);

    // Unset the bit and if need be, push the pagemap back onto the stack
    pagemap_t *pagemap = pagestack.pagemaps + elem_num;
    if(pagemap->bitmap == ULONG_MAX) _pmm_push(&pagestack, pagemap);
    CLEAR_PAGE(pagemap->bitmap, rel_page_num);
    spinlock_irqrelease(&pagestack.lock);
}

bool pmm_alloc_contiguous(paddr_t *addr, size_t pages) {
    pmm_alloc_contiguous_aligned(addr, pages, 1);
}

bool pmm_alloc_contiguous_aligned(paddr_t *addr, size_t pages, unsigned int alignment) {
    // Unfortunately we can't allocate more than #BITS pages contiguously :(
    kassert(pages < BITS);

    pagemap_t *curr, *prev;
    long page = 0;

    spinlock_irqacquire(&pagestack.lock);

    // Loop through the stack until we find a bitmap that has enough contiguous pages
    for(curr = pagestack.top, prev = NULL; curr != NULL; prev = curr, curr = curr->next) {
        // Find the index of the first page in the contiguous set
        page = bitmap_find_contiguous_aligned_zeros(curr->bitmap, pages, alignment);

        // If such a contiguous region of pages exists in the bitmap...
        if(page >= 0) {
            // Then set the bits in the bitmap
            curr->bitmap = bitmap_field_set(curr->bitmap, page, pages);

            // Calculate the page's physical address
            *addr = REL_PAGE_NUM_TO_PADDR(page, (curr - pagestack.pagemaps));

            // Now, if the bitmap is fully allocated...
            if(curr->bitmap == ULONG_MAX) {
                // Remove the pagemap from the stack
                if(prev == NULL) pagestack.top = curr->next;
                else prev->next = curr->next;
                curr->next = NULL;
            }

            return true;
        }
    }

    spinlock_irqrelease(&pagestack.lock);
    return false;
}

void pmm_free_contiguous(paddr_t addr, size_t pages) {
    // Check to make sure addr is page_aligned and within bounds
    kassert(IS_PAGE_ALIGNED(addr) && IS_WITHIN_MEM_BOUNDS(addr));

    // Calculate the absolute page number
    // Get the index into the pagemaps array
    // Get the page number relative to the bitmap in the pagemap
    unsigned long page_num = ATOP(addr);
    unsigned long elem_num = GET_PAGEMAP_ARRAY_INDEX(page_num);
    unsigned long rel_page_num = ABS_TO_REL_PAGE_NUM(page_num, elem_num);

    spinlock_irqacquire(&pagestack.lock);
    pagemap_t *pagemap = pagestack.pagemaps + elem_num;

    // Check that number of contiguous pages to be freed is not greater than what was actually allocated
    size_t actual_contiguous_pages_allocated = _ctz(~(pagemap->bitmap >> rel_page_num));
    kassert(pages <= actual_contiguous_pages_allocated);

    // Unset the bits and if need be, push the pagemap back onto the stack
    if(pagemap->bitmap == ULONG_MAX) _pmm_push(&pagestack, pagemap);
    bitmap_field_clear(pagemap->bitmap, rel_page_num, pages);
    spinlock_irqrelease(&pagestack.lock);
}

void pmm_reserve(paddr_t addr) {
    // Check if the address is page aligned and within bounds
    kassert(IS_PAGE_ALIGNED(addr) && IS_WITHIN_MEM_BOUNDS(addr));

    unsigned long page_num = ATOP(addr);
    unsigned long elem_num = GET_PAGEMAP_ARRAY_INDEX(page_num);
    unsigned long rel_page_num = ABS_TO_REL_PAGE_NUM(page_num, elem_num);

    spinlock_irqacquire(&pagestack.lock);

    pagemap_t *pagemap = pagestack.pagemaps + elem_num;

    // The bitmap must not already be full
    kassert(pagemap->bitmap != ULONG_MAX);

    // Set the bit and if need be, remove the pagemap
    SET_PAGE(pagemap->bitmap, rel_page_num);

    // Removing the pagemap from the stack is a little more involved...In most cases we won't need to do this
    if(pagemap->bitmap == ULONG_MAX) {
        // If the pagemap isn't the top of the stack then Loop through the stack until we find the previous pagemap
        pagemap_t *curr = pagestack.top;
        if(curr == pagemap) {
            _pmm_pop(&pagestack);
        } else {
            for(; curr->next != pagemap; curr = curr->next);
            curr->next = pagemap->next;
            pagemap->next = NULL;
        }
    }

    spinlock_irqrelease(&pagestack.lock);
}
