#include <kernel/pmap.h>

#include <platform/interrupts.h>

#include <string.h>

// A page directory has 4096 page directory entries
#define PGDNENTRIES 4096
// A page table has 256 page table entries
#define PGTNENTRIES 256

// Get the index offsets into the pgd and pgt from the virtual address
#define PGD_GET_INDEX(B) (((B) & 0xFFF00000) >> 20)
#define PGT_GET_INDEX(B) (((B) & 0x000FF000) >> 12)

// Macros to set and clear bits from the pte
#define PTE_SET_BIT(entry, bit) (entry) |= (bit)
#define PTE_CLEAR_BIT(entry, bit) (entry) &= ~((bit))

// Construct a pte
// addr is the physical address of the page to be mapped and must be page
// aligned. Bits are the control bits to set in the pte
#define PTE_CREATE(addr, bits) (PTE_PAGE_BIT | (addr) | (bits))

// This bit indicates that the pte represents a small page
#define PTE_PAGE_BIT (0x2)

// Not global bit - This bit indicates whether the pte should be treated as
// global in the TLB. Global pte's are not flushed when flushing entries
// corresponding to a certain ASID from the TLB
#define PTE_NG_BIT (0x800)
// Shareability bit - This bit indicates whether the page is 'shareable'
// meaning whether the caches between multiple CPUs should be synchronized
// whenever data from this page has been modified
#define PTE_S_BIT (0x400)

// This bit can be used as a 'used' or 'accessed' flag
#define PTE_AP0_BIT (0x10)
// Enable PL0 (user mode) access
#define PTE_AP1_BIT (0x20)
// Disable write access (all modes)
#define PTE_AP2_BIT (0x200)

// Execute never bit - This bit indicates that the data in the page should not
// be executed
#define PTE_XN_BIT (0x1)

// Shareability domain - If set, indicates inner shareability (which is the 
// usual case)
#define PTE_TEX0_BIT (0x40)

// Strongly ordered memory when TEX0=0
// 	outer non-cacheable
// 	inner non-cacheable
// Normal memory when TEX0=1
// 	outer non-cacheable
// 	inner non-cacheable
#define PTE_CB0 (0x0)
// Device memory when TEX0=0
// 	outer non-cacheable
// 	inner non-cacheable
// Normal memory when TEX0=1
// 	outer write-back, write-allocate
// 	inner write-through, no write-allocate
#define PTE_CB1 (0x4)
// Normal memory when TEX0=0
// 	outer write-through, no write-allocate
// 	inner write-through, no write-allocate
// Strongly ordered memory when TEX0=1
// 	outer non-cacheable
// 	inner non-cacheable
#define PTE_CB2 (0x8)
// Normal memory when TEX0=0
// 	outer write-back, no write-allocate
// 	inner write-back, no write-allocate
// Normal memory when TEX0=1
// 	outer write-back, write-allocate
// 	inner write-back, write-allocate
#define PTE_CB3 (0xC)

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

vaddr_t _pmap_bootstrap_memory(size_t size) {
	// pmap_init must be called before this function can be used, otherwise
	// kernel_vend will be an incorrect value
	// kernel_vend and kernel_pend should be page aligned
	static vaddr_t placement_addr = 0;
	placement_addr = (placement_addr == 0) ? kernel_vend : placement_addr;

	vaddr_t start = placement_addr;
	vaddr_t end = placement_addr + size;

	// Allocate a new page if there is not enough memory
	if(end >= kernel_vend) {
		// Start of page table array
		pgt_t *pgts = (pgt_t*)(&__pgt_virtual_start);

		// Kernel page tables start at this entry number in the page directory
		uint32_t pgt_n = PGDNENTRIES - (uint32_t)(&__pgt_num);

		// Loop through and map the pages while incrementing kernel_pend and 
		// kernel_vend 
		for(; kernel_vend < end; kernel_vend+=PAGESIZE, kernel_pend+=PAGESIZE) {
			pte_t entry = PTE_CREATE(kernel_pend, PTE_XN_BIT|PTE_S_BIT|PTE_TEX0_BIT|PTE_CB3);
			pgts[PGD_GET_INDEX(kernel_vend)-pgt_n].pte[PGT_GET_INDEX(kernel_vend)] = entry;
		}
	}

	// Zero the memory
	memset((void*)placement_addr, 0x0, size);

	placement_addr = end;

	return(start);
}

// TODO: Should a lock be used to access kernel_pmap?
// Setup the kernel's pmap
void _pmap_kernel_init() {
	// Set the end of the kernel's virtual and physical address space
	kernel_vend = ROUND_PAGE((vaddr_t)(&__pgt_virtual_start) + sizeof(pgt_t) * 
		(vaddr_t)(&__pgt_num));
	kernel_pend = ROUND_PAGE((paddr_t)(&__pgt_physical_start) + sizeof(pgt_t) * 
		(paddr_t)(&__pgt_num));

	// The kernel's pgd has already been set up and we know where it is via the
	// linker script symbol
	kernel_pmap.pgd = (pgd_t*)(&__pgd_virtual_start);

	// Need to allocate memory for the pgt_entry structs
	// We are too early in the bootstrap process to be able to use the heap
	// so we need to use _pmap_bootstrap_memory
	uint32_t n_pgt = (uint32_t)(&__pgt_num);
	pgt_entry_t *pentries = (pgt_entry_t*)_pmap_bootstrap_memory(sizeof(pgt_entry_t)
			* n_pgt);

	kernel_pmap.pgt_entry_head = pentries;

	// The kernel's page tables have already been setup and we know where they
	// are located via the linker script symbols
	pgt_t *pg_tables = (pgt_t*)(&__pgt_virtual_start);

	for(uint32_t i = 0; i < n_pgt; i++) {
		// Get the location of the page table
		pentries[i].pgt = &pg_tables[i];

		// Assign the next pgt_entry
		pentries[i].next = ((i+1) < n_pgt) ? &pentries[i+1] : NULL;

		// The kernel virtual address space is always the last n MB, thus the 
		// page tables will always be the mapped to the last n entries in the 
		// page directories. Where n == n_pgt
		pentries[i].offset = PGDNENTRIES-n_pgt+i;
	}

	// Count the resident and wired pages for the kernel (will be the same)
	uint32_t n_tot_entries = n_pgt * PGTNENTRIES;
	uint32_t *pte = (uint32_t*)pg_tables;
	for(uint32_t i = 0; i < n_tot_entries; i++) {
		if(pte[i] != 0) {
			kernel_pmap.pmap_stats.wired_count++;
			kernel_pmap.pmap_stats.resident_count++;
		}
	}

	// Now remove the identity mapped bottom 1 MB section in the pgd
	kernel_pmap.pgd->pde[0] = 0x0;
}

void pmap_init() {
	// Initialize the kernel pmap
	_pmap_kernel_init();
}

// TODO: Should a lock be used to access kernel_pmap?
// TODO: Assert valid vaddr (must be in kernel high memory)
// TODO: Assert memory available (what if no more high memory left?), panic 
void pmap_kenter_pa(vaddr_t vaddr, paddr_t paddr, vm_prot_t vm_prot, pmap_flags_t pmap_flags) {
  // vaddr must be in the kernel virtual address space (i.e. >= 0xF0010000)
	// Encode the protection and pmap flags in the page table entry 
	uint32_t pte_flags = 0;
  pte_flags |= (vm_prot & VM_PROT_EXECUTE) ? pte_flags : PTE_XN_BIT;
	pte_flags |= (vm_prot & VM_PROT_WRITE) ? pte_flags : PTE_AP2_BIT;
	
  pte_flags |= (pmap_flags & PMAP_WRITE_BACK) ? pte_flags : (PTE_TEX0_BIT | PTE_CB3);
	pte_flags |= (pmap_flags & PMAP_WRITE_COMBINE) ? pte_flags : (PTE_TEX0_BIT | PTE_CB0);
	pte_flags |= (pmap_flags & PMAP_NOCACHE_OVR) ? pte_flags : (PTE_CB1 | PTE_XN_BIT);
	pte_flags |= (pmap_flags & PMAP_NOCACHE) ? pte_flags : (PTE_CB1 | PTE_XN_BIT);
	
  pte_t entry = PTE_CREATE(paddr, PTE_S_BIT | pte_flags);

  // Now we must place the page table entry in the correct kernel page table
  // Since we know that the pgts are laid out contiguously in memory we can cheat by
  // accessing the correct pgt directly without having to loop over the pmap_kernel's pgt_entries
  // to search for the pgt_entry with the correct offset in the pgd.

  // Start of the page table array
  pgt_t *pgts = (pgt_t*)(&__pgt_virtual_start);

  // Kernel page tables start at this entry number in the page directory
  uint32_t pgt_n = PGDNENTRIES - (uint32_t)(&__pgt_num);

  // Place the entry in the page table
  pgts[PGD_GET_INDEX(vaddr)-pgt_n].pte[PGT_GET_INDEX(vaddr)] = entry;

  // Update the stats
  pmap_kernel()->pmap_stats.wired_count++;
  pmap_kernel()->pmap_stats.resident_count++;
}

// TODO: fix this to use pmap_kenter_pa
vaddr_t pmap_steal_memory(size_t size) {
	// pmap_init must be called before this function can be used, otherwise
	// kernel_vend will be an incorrect value
	// kernel_vend and kernel_pend should be page aligned
  // This function should only be used before pmm_init is called
	static vaddr_t placement_addr = 0;
	placement_addr = (placement_addr == 0) ? kernel_vend : placement_addr;

	vaddr_t start = placement_addr;
	vaddr_t end = placement_addr + size;

	// Allocate a new page if there is not enough memory
	if(end >= kernel_vend) {
    // Loop through and map the pages using pmap_kenter_pa while incrementing kernel_pend and kernel_vend
    for(; kernel_vend < end; kernel_vend+=PAGESIZE, kernel_pend+=PAGESIZE) {
      pmap_kenter_pa(kernel_vend, kernel_pend,  VM_PROT_DEFAULT, PMAP_WRITE_BACK);
    }
	}

	// Zero the memory
	memset((void*)placement_addr, 0x0, size);

	placement_addr = end;

	return(start);
}

