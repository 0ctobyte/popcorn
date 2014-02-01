#include <kernel/pmap.h>
#include <kernel/pmm.h>

#include <mach/interrupts.h>

// Page directory entries and page table entries are both 32 bits
typedef uint32_t pde_t;
typedef uint32_t pte_t;

// A page table has 256 page table entries
typedef struct {
	pte_t pte[256];
} pgt_t;

// A page directory has 4096 page directory entries
struct pgd {
	pde_t pde[4096];
};

// This is needed for the pmap struct
struct pgt_entry {
	// Pointer to page table struct
	pgt_t *pgt;
	// Pointer to next entry
	struct pgt_entry *next;
	// Offset into page directory where the page table is mapped
	uint32_t offset;
};

// Kernel pmap
pmap_t kernel_pmap;

// Linker symbols
extern void __kernel_virtual_start();
extern void __kernel_physical_start();
extern void __text_virtual_start();
extern void __text_physical_start();
extern void __text_virtual_end();
extern void __text_physical_end();
extern void __data_virtual_start();
extern void __data_physical_start();
extern void __data_virtual_end();
extern void __data_physical_end();
extern void __kernel_virtual_end();
extern void __kernel_physical_end();
extern void __pgd_virtual_start();
extern void __pgd_physical_start();
extern void __pgt_num();

void _pmap_kernel_init() {
}

void pmap_init() {
	// Initialize the kernel pmap
	_pmap_kernel_init();	
}

