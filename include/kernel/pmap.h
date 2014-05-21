#ifndef __PMAP_H__
#define __PMAP_H__

#include <sys/types.h>
#include <kernel/mm_types.h>

// Flag bits
typedef uint32_t pmap_flags_t;

#define PMAP_WIRED (0x8)
#define PMAP_CANFAIL (0x10)
#define PMAP_NOCACHE (0x20)
#define PMAP_WRITE_COMBINE (0x40)
#define PMAP_WRITE_BACK (0x80)
#define PMAP_NOCACHE_OVR (0x100)
#define PMAP_DEVICE_MEM (0x200)

// Opaque types used in pmap_t
typedef struct pgd_struct pgd_t;
typedef struct pgt_entry pgt_entry_t;

// These should be updated during any pmap function calls if necessary
typedef struct {
	size_t wired_count;
	size_t resident_count;
} pmap_statistics_t;

typedef struct {
	// Page directory is the 1st level translation table on ARMv7
	pgd_t *pgd;

	// Page tables are the 2nd level translation tables on ARMv7
	// Page tables will be allocated on demand and when a new table is created
	// it will be added to this list.
	// To find which page table to a vaddr_t translate too, we will need to 
	// use the physical address of the page table from the page directory and
	// iterate through the list comparing the physical address with that of
	// pmap_extract.
	pgt_entry_t *head;

	pmap_statistics_t pmap_stats;
} pmap_t;

// Declare the kernel's pmap
extern pmap_t kernel_pmap;

// Initializes the pmap system
void pmap_init();

// Creates a pmap_t struct and returns it. The reference count on the pmap
// will be set to 1
pmap_t* pmap_create();

// Drops the reference count on the pmap. If it becomes 0, then all resources
// allocated for the pmap are destroyed
void pmap_destroy(pmap_t*);

// Increments the reference count on the specified pmap
void pmap_reference(pmap_t*);

// Returns the # of pages resident in the pmap
#define pmap_resident_count(pmap) ((pmap)->pmap_stats.resident_count)

// Returns the # of pages wired in the pmap
#define pmap_wired_count(pmap) ((pmap)->pmap_stats.wired_count)

// Adds a virtual to physical page mapping to the specified pmap using the 
// specified protection
uint32_t pmap_enter(pmap_t*, vaddr_t, vm_prot_t, pmap_flags_t);

// Removes a range of virtual to physical page mappings from the specified pmap
void pmap_remove(pmap_t*, vaddr_t, vaddr_t);

// Removes all mappings from the pmap
void pmap_remove_all(pmap_t*);

// Changes the protection of all mappings in the specified range in the pmap
void pmap_protect(pmap_t *pmap, vaddr_t, vaddr_t, vm_prot_t);

// Clears the wired attribute on the mapping for the specified virtual address
void pmap_unwire(pmap_t*, vaddr_t);

// Extracts the mapping for the specified virtual address, i.e. it gets the 
// physical address associated with a virtual address. Returns false if no such
// mapping exists
bool pmap_extract(pmap_t*, vaddr_t, paddr_t*);

// Copies page mappings from pmap to another
void pmap_copy(pmap_t *dst, pmap_t *src, vaddr_t *d, size_t len, vaddr_t *s);

// Activate the pmap, i.e. set the translation table base register with the
// page directory associated with the pmap
void pmap_activate(pmap_t*);

// Deactivate the pmap
void pmap_deactivate(pmap_t*);

// Zeros out the page at the specified physical address
void pmap_zero_page(paddr_t);

// Copies the page specified at physical address src to physical address dst
void pmap_copy_page(paddr_t src, paddr_t dst);


// Pmap kernel functions

// Returns the kernel's pmap; must return a reference to kernel_pmap_ptr
#define pmap_kernel() (&(kernel_pmap))

// Used to determine the kernel's virtual address space start and end
void pmap_virtual_space(vaddr_t *start, vaddr_t *end);

// Enter an unmanaged mapping for the kernel pmap
// This mapping will not be affected by other systems and will always be wired
void pmap_kenter_pa(vaddr_t, paddr_t, vm_prot_t, pmap_flags_t);

// Removes all mappings starting at the specified virtual address to the size
// (in bytes) specified from the kernel pmap. All mappings must have been
// entered with pmap_kenter_pa
void pmap_kremove(vaddr_t, size_t);

// Clears the modified attribute of a page
//pmap_clear_modify

// Tests the referenced attribute of a page
//pmap_is_referenced

// Tests the modified attribute of a page
//pmap_is_modified

// Clears the referenced attribute of a page
//pmap_clear_reference

//pmap_page_protect*
//pmap_update*

#endif // __PMAP_H__
