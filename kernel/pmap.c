#include <kernel/pmap.h>
#include <kernel/pmm.h>

#include <mach/interrupts.h>

// A page directory has 4096 page directory entries
#define PGDNENTRIES 4096
// A page table has 256 page table entries
#define PGTNENTRIES 256

// Page directory entries and page table entries are both 32 bits
typedef uint32_t pde_t;
typedef uint32_t pte_t;

// Page tables are the 2nd level translation table used on ARMv7 CPU's
// An entry in the page table contains the physical address of the page in 
// memory as well as other info (access permissions etc.)
typedef struct {
	pte_t pte[PGTNENTRIES];
} pgt_t;

// A page directory is the 1st level translation table used in an ARMv7 CPU
// A page directory entry contains the physical address of 2nd level page
// tables as well as other control bits
struct pgd_struct {
	pde_t pde[PGDNENTRIES];
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
extern uintptr_t __kernel_virtual_start;
extern uintptr_t __kernel_physical_start;
extern uintptr_t __text_virtual_start;
extern uintptr_t __text_physical_start;
extern uintptr_t __text_virtual_end;
extern uintptr_t __text_physical_end;
extern uintptr_t __data_virtual_start;
extern uintptr_t __data_physical_start;
extern uintptr_t __data_virtual_end;
extern uintptr_t __data_physical_end;
extern uintptr_t __kernel_virtual_end;
extern uintptr_t __kernel_physical_end;
extern uintptr_t __pgd_virtual_start;
extern uintptr_t __pgd_physical_start;
extern uintptr_t __pgt_virtual_start;
extern uintptr_t __pgt_physical_start;
extern uintptr_t __pgt_num;

// These are set to the end of the kernel's virtual address space and physical
// address space
paddr_t kernel_pend;
vaddr_t kernel_vend;

#include <kernel/kstdio.h>
void _pmap_kernel_init() {
	// Set the end of the kernel's virtual and physical address space
	kernel_pend  = (paddr_t)(&__pgd_physical_start) + sizeof(pgd_t) + 
		sizeof(pgt_t) * (paddr_t)(&__pgt_num);
	kernel_vend = (vaddr_t)(&__pgd_virtual_start) + sizeof(pgd_t) +
		sizeof(pgt_t) * (vaddr_t)(&__pgt_num);

	// The kernel's pgd has already been set up and we know where it is via the
	// linker script symbol
	kernel_pmap.pgd = (pgd_t*)(&__pgd_virtual_start);
}

void pmap_init() {
	// Initialize the kernel pmap
	_pmap_kernel_init();	
}

