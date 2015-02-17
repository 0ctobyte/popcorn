#include <kernel/pmm.h>
#include <kernel/kassert.h>
#include <kernel/spinlock.h>
#include <kernel/pmap.h>

#include <platform/interrupts.h>

#include <lib/bithacks.h>

// Set a bit and clear a bit in the bitmap
#define SET_FRAME(bitmap, rel_frame_num) (bitmap) |= (1 << (rel_frame_num))
#define CLEAR_FRAME(bitmap, rel_frame_num) (bitmap) &= ~(1 << (rel_frame_num))

// Shift by the # of low order bits that must be zero in order to be 
// page aligned. This gives the absolute frame number.
// Multiply pagemap array index by BITS and subtract value from absolute
// frame number to get relative frame number within a bitmap.
#define GET_REL_FRAME_NUM(abs_frame_num, elem_num) ((abs_frame_num) - ((elem_num) * BITS))

// The number of bits in each bitmap
#define BITS (32)

// Shift by the log base 2 of the number of BITS. This gives the index into
// the pagemaps array
#define GET_PAGEMAP_ARRAY_INDEX(abs_frame_num) ((abs_frame_num) >> (5))

/*
 * So how does this physical memory allocator work?
 * The basic idea is that main memory is split into fixed sized partitions 
 * called 'page frames' (or just 'frames'...or 'pages'). This allocator then
 * simply hands out such frames and frees them when asked to do so. Also, the
 * allocator should be able to allocate a contiguous range of free frames.
 * 
 * One of the popular ways to do this is to use a bitmap, an array of bits, 
 * where each bit represents a frame in memory. In terms of speed, this method
 * is not very efficient since operations such as allocating a frame, freeing 
 * a frame and allocating a contiguous range of frames would all take O(n) time
 * (n is the number of frames in memory). 
 *
 * Another popular method is to use a stack of free frames. The stack will hold
 * the physical addresses to each free frame. Thus, when allocating a frame,
 * the first frame on the stack is simply popped off. Similarly, when freeing a
 * frame, the address of that frame is pushed on to the stack. With a stack,
 * allocating and freeing a frame takes constant time. This is great! However,
 * there are some problems.
 *
 * Specifically, memory use and allocating a contiguous range of free frames.
 * Assuming we are on a 32-bit system with the maximum 4 Gb of RAM and a
 * page size of 4 kb, a bitmap would take about 128 kb of memory. A stack on the
 * other hand, where a memory address takes about 4 bytes, would take 4 Mb of 
 * memory. Not only that but trying to allocate a continuous range of free
 * frames using the stack method is non-trivial. 
 *
 * So what now? Well, why not combine the best of stacks and bitmaps!!one!1!?
 * So here's the idea on how this physical memory allocator works. We will have
 * a struct which I call a 'pagemap'. This pagemap will have a bitmap with 32
 * bits and a pointer to the next (in the stack) pagemap. This pagemap will be
 * 8 bytes in size. There will be an array of these pagemaps, the size of the
 * array depending on the size of the memory and the page size. For example, on
 * a 4 Gb system with a page size of 4 kb, there will be 32768 of these 
 * pagemaps which means 256 kb will be used for this array. Now, each bit in
 * the bitmaps of each pagemap represents one page. Okay, so initially each of
 * these pagemaps will be linked via the next pointer (the last pagemap in the
 * array of course will have a NULL next pointer). This linked array is our
 * stack, the top of the stack is the first element in the array INITIALLY. 
 * This will change as frames are allocated and freed. Now, when we want to
 * allocate a frame we simple look at the first pagemap in the stack, find
 * a free bit in the 32-bit bitmap and calculate the address using the position
 * of the pagemap in the array (element number) and the position of the free
 * bit in the bitmap. For example, the 133rd frame would be represented by the
 * 5th bit (133-32*floor(133/32))in the 4th pagemap (floor(133/32)) in the
 * array. If the bitmap has been fully set after allocating the page then we
 * simply pop it off the stack. When freeing a frame, we use the frame address
 * to calculate the position of the pagemap in the array and position of the
 * bit in the bitmap of that pagemap and then unset it. If that bitmap was
 * previously fully allocated then we simply push it on the pagemap stack.
 * 
 * Thus, using this method, which I call the 'bitstack' method :), 
 * allocating and freeing a single frame takes O(1) time. So what about 
 * contiguous frame allocation and memory use? Well I mentioned above that
 * this method takes 256 kb of memory max on a 32 bit system with a page size
 * of 4 kb (standard). That's just double the memory use of a bitmap (128 kb)
 * and compared to the stack method (4 Mb), 256 kb looks great! Now for 
 * contiguous frame allocation. Contiguous frame allocation is still not
 * very efficient using this method as it will take O(n) time where n is 
 * the number of PAGEMAPS in the array. The pmm_alloc_contiguous function
 * below uses a function, bit_find_contiguous_zeros, which employs a for loop.
 * However, this for loop NEVER loops more than 32 times MAX. And it would
 * rarely ever loop 32 times (in most cases it will loop less than half that
 * number). Thus, O(n) is a valid estimate of pmm_alloc_contiguous (in case
 * anyone was wondering).
 *
 * However, using the bitmap method, contiguous frame allocation takes O(m) time
 * where m is the number of FRAMES. The number of pagemaps in the array will
 * always be less than the number of frames in memory. Thus, the bitstack
 * method is able to allocate contiguous frames faster than using the bitmap
 * method. Contiguous frame allocation using the stack method, disregarding the
 * complexity of code and logic required to implement such a function, would
 * take O(m) time as well.
 *
 * Thus, with the bitstack method, we get similar performance for allocating
 * and freeing a frame as we do with the stack method, memory efficiency
 * comparable to the bitmap method and faster contiguous frame allocation
 * than both the bitmap and stack method! However, the only downside of the
 * bitstack method is that only 32 contiguous frames can be allocated at once
 * (since the pagemaps are in a contiguous array, there could be ways to 
 * circumvent this limit).
 * For the purposes of this kernel, this limit on the contiguous frames
 * shouldn't be much of a problem.
 */

typedef struct pagemap_t {
	uint32_t bitmap; // Each bit represents one page
	struct pagemap_t *next;  // Pointer to next pagemap in the stack
} pagemap_t;

typedef struct {
	spinlock_t lock;
	pagemap_t *pagemaps; // Array of pagemap_t
	pagemap_t *top;	     // Top of stack
	size_t size;         // # of pagemap_t in the pagemap_t array
} pagestack_t;

static pagestack_t pagestack = {SPINLOCK_INIT, 0, 0, 0};

// Pushes the given pagemap on the stack
void _pmm_push(pagestack_t *pstack, pagemap_t *pagemap) {
	pagemap->next = pstack->top;
	pstack->top = pagemap;
}

// Pops the pagemap off the stack
pagemap_t* _pmm_pop(pagestack_t *pstack) {
	if(pstack->top == NULL) return(NULL);
	pagemap_t *next = pstack->top->next;
	pagemap_t *popped = pstack->top;
	pstack->top->next = NULL;
	pstack->top = next;
	return(popped);
}

void pmm_init() {
	// Allocate memory for the pagemaps
	// pmap_steal_memory will only work if pmap_init has been called before
	// Ideally, pmap_init will pmm_init
	pagestack.size = ((MEMSIZE/PAGESIZE)/BITS);
	pagestack.pagemaps = (pagemap_t*)pmap_steal_memory(pagestack.size * sizeof(pagemap_t));

	// Link the pagemaps together
	pagestack.top = pagestack.pagemaps;
	pagemap_t *p = pagestack.pagemaps;
	for(uint32_t i = 0; i < pagestack.size; ++i) {
		// Make sure to clear the bitmaps
		p[i].bitmap = 0;
		p[i].next = ((i+1) < pagestack.size) ? &p[i+1] : NULL;
	}
}

paddr_t pmm_alloc() {
	// Default value of addr, this indicates that no free frame was found
	paddr_t addr = UINTPTR_MAX;
	
	spin_irqlock(&pagestack.lock);

	pagemap_t *top = pagestack.top;
	if(top == NULL) return(addr);

	// Find first free frame in pagemap and set the bit in the bitmap
	// Calculate address using location of pagemap in the pagestacks.pagemaps
	// array and the bit number in the pagemap's bitmap
	uint32_t frame = bit_find_contiguous_zeros(top->bitmap, 1);
	SET_FRAME(top->bitmap, frame);
	addr = (((top - pagestack.pagemaps) * BITS  + frame) * PAGESIZE) + MEMBASEADDR;

	// If pagemap is fully allocated, pop it off the stack
	if(top->bitmap == UINT32_MAX) _pmm_pop(&pagestack);
	spin_irqunlock(&pagestack.lock);
	return(addr);
}

void pmm_free(paddr_t addr) {
	// Check to make sure addr is page_aligned and within bounds
	kassert(IS_PAGE_ALIGNED(addr) && IS_WITHIN_BOUNDS(addr));

	// Calculate the absolute frame number
	// Get the index into the pagemaps array
	// Get the frame number relative to the bitmap in the pagemap
	uint32_t frame_num = ATOP((addr-MEMBASEADDR));
	uint32_t elem_num = GET_PAGEMAP_ARRAY_INDEX(frame_num);
	uint32_t rel_frame_num = GET_REL_FRAME_NUM(frame_num, elem_num);

	spin_irqlock(&pagestack.lock);

	// Unset the bit and if need be, push the pagemap back onto the stack
	pagemap_t *pagemap = pagestack.pagemaps + elem_num;
	if(pagemap->bitmap == UINT32_MAX) _pmm_push(&pagestack, pagemap);
	CLEAR_FRAME(pagemap->bitmap, rel_frame_num);
	spin_irqunlock(&pagestack.lock);
}

paddr_t pmm_alloc_contiguous(size_t frames) {
	// Unfortunately we can't allocate more than 32 frames contiguously :(
	kassert(frames < BITS);

	paddr_t addr = UINTPTR_MAX;

	pagemap_t *curr, *prev;
	int32_t frame = 0;

	spin_irqlock(&pagestack.lock);

	// Loop through the stack until we find a bitmap that has enough 
	// contiguous frames
	for(curr = pagestack.top, prev = NULL; curr != NULL; prev = curr, curr = curr->next) {
		// Find the index of the first frame in the contiguous set
		frame = bit_find_contiguous_zeros(curr->bitmap, frames);
		
		// If such a contiguous region of frames exists in the bitmap...
		if(frame >= 0) {
			// Then set the bits in the bitmap
			curr->bitmap = bit_field_set(curr->bitmap, frame, frames);
			
			// Calculate the page's physical address
			addr = ((curr - pagestack.pagemaps) * BITS + frame) * PAGESIZE;
			
			// Now, if the bitmap is fully allocated...
			if(curr->bitmap == UINTPTR_MAX) {
				// Remove the pagemap from the stack
        if(prev == NULL) pagestack.top = curr->next;
        else prev->next = curr->next;
				curr->next = NULL;
			}
			break;
		}
	}

	spin_irqunlock(&pagestack.lock);
	return(addr);
}

void pmm_reserve(paddr_t addr) {
	// Check if the address is page aligned and within bounds
	kassert(IS_PAGE_ALIGNED(addr) && IS_WITHIN_BOUNDS(addr));

	uint32_t frame_num = ATOP((addr-MEMBASEADDR));
	uint32_t elem_num = GET_PAGEMAP_ARRAY_INDEX(frame_num);
	uint32_t rel_frame_num = GET_REL_FRAME_NUM(frame_num, elem_num);

	spin_irqlock(&pagestack.lock);

	pagemap_t *pagemap = pagestack.pagemaps + elem_num;
	
	// The bitmap must not already be full
	kassert(pagemap->bitmap != UINT32_MAX);

	// Set the bit and if need be, remove the pagemap
	SET_FRAME(pagemap->bitmap, rel_frame_num);
	
	// Removing the pagemap from the stack is a little more involved...In most
	// cases we won't need to do this
	if(pagemap->bitmap == UINT32_MAX) {
		// If the pagemap isn't the top of the stack then...
		// Loop through the stack until we find the previous pagemap
		pagemap_t *curr = pagestack.top;
		if(curr == pagemap) {
			_pmm_pop(&pagestack);
		} else {
			for(; curr->next != pagemap; curr = curr->next);
			curr->next = pagemap->next;
			pagemap->next = NULL;
		}
	}

	spin_irqunlock(&pagestack.lock);
}	

