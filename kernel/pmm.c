#include <kernel/pmm.h>
#include <kernel/kassert.h>
#include <kernel/interrupts.h>

#define PAGESIZE 0x1000 // TODO: TEMPORARY
#define MEMSIZE 0x8000000 // TODO: TEMPORARY

// TODO: We need to make this not depended on a 4 kb page size
#define IS_PAGE_ALIGNED(B) ((B & 0xFFF) == 0)
#define IS_WITHIN_BOUNDS(B) ((B < MEMSIZE))

// # of low order bits that must be zero in order to be page aligned
#define PAGELOWBITSZERO (12)

// The number of bits in each bitmap
#define BITS 32

// log base 2 of the number of BITS
#define LOG2BITS 5

typedef struct pagemap_t {
	uint32_t bitmap; // Each bit represents one page
	struct pagemap_t *next;  // Pointer to next pagemap in the stack
} pagemap_t;

typedef struct {
	pagemap_t *pagemaps; // Array of pagemap_t
	// Top of stacks
	// top[0] - pagemaps with a single free contiguous frame
	// top[1] - pagemaps with 2 free contiguous frames
	// top[2] - pagemaps with 4 free contiguous frames
	// top[3] - pagemaps with 8 free contiguous frames
	// top[4] - pagemaps with 16 free contiguous frames
	// top[5] - pagemaps with 32 free contiguous frames
	pagemap_t *top[6];
	size_t size;         // # of pagemap_t in the pagemap_t array
} pagestack_t;

pagestack_t pagestack = {0, {0, 0, 0, 0, 0, 0}, 0};

pagemap_t** _pmm_get_stack_top(pagestack_t *pstack) {
	uint32_t i;
	for(i = 0; pstack->top[i] == NULL && i < 6; i++);
	if(i < 6) return(&pstack->top[i]);
	else return(NULL);
}

// Returns the frame number, relative to the given bitmap, of the first free
// frame in the bitmap
uint32_t _pmm_first_free_frame(pagemap_t *pagemap) {
	uint32_t bitmap = pagemap->bitmap;
	uint32_t frame = 0;
	for(frame = 0; frame < BITS; ++frame, bitmap >>= 1) {
		if(0x1 & (~bitmap)) break;
	}
	return(frame);
}

// Pushes the given pagemap on the stack
void _pmm_push(pagemap_t **top, pagemap_t *pagemap) {
	pagemap->next = (*top);
	*top = pagemap;
}

// Pops the pagemap off the stack
void _pmm_pop(pagemap_t **top) {
	pagemap_t *next = (*top)->next;
	(*top)->next = NULL;
	*top = next;
}

address_t pmm_alloc() {
	INTERRUPT_LOCK();
	// Default value of addr, this indicates that no free frame was found
	address_t addr = UINTPTR_MAX;
	pagemap_t **top = _pmm_get_stack_top(&pagestack);
	if(top == NULL) return(addr);

	// Find first free frame in pagemap and set the bit in the bitmap
	// Calculate address using location of pagemap in the pagestacks.pagemaps
	// array and the bit number in the pagemap's bitmap
	uint32_t frame = _pmm_first_free_frame(*top);
	(*top)->bitmap |= (1 << frame);
	addr = (((*top) - pagestack.pagemaps) * BITS  + frame) * PAGESIZE;

	// If pagemap is fully allocated, pop it off the stack
	if((*top)->bitmap == UINT32_MAX) _pmm_pop(top);
	INTERRUPT_UNLOCK();
	return(addr);
}

void pmm_free(address_t addr) {
	INTERRUPT_LOCK();
	// Check to make sure addr is page_aligned and within bounds
	if(IS_PAGE_ALIGNED(addr) && IS_WITHIN_BOUNDS(addr)) {
		// Calculate the absolute frame number
		uint32_t frame_num = addr >> PAGELOWBITSZERO;

		// This gets the index into the pagemaps array
		uint32_t elem_num = frame_num >> LOG2BITS;

		// This gets the frame number relative to the bitmap in the pagemap
		uint32_t rel_frame_num = frame_num - (elem_num * 32);

		// Unset the bit and if need be, push the pagemap back onto the stack
		pagemap_t *pagemap = pagestack.pagemaps + elem_num;
		if(pagemap->bitmap == UINT32_MAX) _pmm_push(&pagestack.top[0], pagemap);
		pagemap->bitmap &= ~(1 << rel_frame_num);
	}
	INTERRUPT_UNLOCK();
}

address_t pmm_alloc_contiguous(size_t num_frames, uint32_t alignment) {
	INTERRUPT_LOCK();

	address_t addr = UINTPTR_MAX;

	// Unfortunately we can't allocate more than 32 frames contiguously :(
	if(num_frames > 32 || alignment > 32) return(addr);

	

	INTERRUPT_UNLOCK();
	return(num_frames*alignment);
}
