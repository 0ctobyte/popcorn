#ifndef __PMAP_H__
#define __PMAP_H__

#include <sys/types.h>
#include <kernel/vmm.h>
#include <kernel/spinlock.h>
#include <kernel/atomic.h>
#include <kernel/vm_page.h>

// Flag bits. The READ, WRITE and EXECUTE bits indicate the page is being mapped in because of those
// access types.
typedef unsigned long pmap_flags_t;

#define PMAP_FLAGS_READ          (0x1)
#define PMAP_FLAGS_WRITE         (0x2)
#define PMAP_FLAGS_EXECUTE       (0x3)
#define PMAP_FLAGS_WIRED         (0x8)
#define PMAP_FLAGS_CANFAIL       (0x10)
#define PMAP_FLAGS_NOCACHE       (0x20)
#define PMAP_FLAGS_WRITE_COMBINE (0x40)
#define PMAP_FLAGS_WRITE_BACK    (0x80)
#define PMAP_FLAGS_NOCACHE_OVR   (0x100)

// These should be updated during any pmap function calls if necessary
typedef struct {
    size_t wired_count;
    size_t resident_count;
} pmap_statistics_t;

typedef struct {
    spinlock_t lock;
    paddr_t ttb;                     // Translation table base address
    unsigned int asid;               // ASID associated with this pmap
    atomic_t refcnt;                 // Reference count on the pmap
    pmap_statistics_t stats;
} pmap_t;

// Declare the kernel's pmap
extern pmap_t kernel_pmap;

// Early init bootstrap code. This functions gets the kernel to run in virtual memory mode
void pmap_bootstrap(void);

// Initializes the pmap system
void pmap_init(void);

// Used to determine the kernel's virtual address space start and end that will be managed by the vmm
void pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp);

// This function should only be used as a bootstrap memory allocator before the memory management systems have been setup.
// It will allocate the required memory if available and map it into the kernel's address space
vaddr_t pmap_steal_memory(size_t vsize, vaddr_t *vstartp, vaddr_t *vendp);

// Returns the kernel's pmap; must return a reference to kernel_pmap
#define pmap_kernel() (&(kernel_pmap))

// Creates a pmap_t struct and returns it. The reference count on the pmap will be set to 1
pmap_t* pmap_create(void);

// Drops the reference count on the pmap. If it becomes 0, then all resources allocated for the pmap are destroyed
void pmap_destroy(pmap_t *pmap);

// Increments the reference count on the specified pmap
void pmap_reference(pmap_t *pmap);

// Returns the # of pages resident in the pmap
#define pmap_resident_count(pmap) ((pmap)->pmap_stats.resident_count)

// Returns the # of pages wired in the pmap
#define pmap_wired_count(pmap) ((pmap)->pmap_stats.wired_count)

// Adds a virtual to physical page mapping to the specified pmap using the specified protection
// The flags contain the access type to the page specified by PA in order to indicate what type of access caused this page to be mapped; these are the same bits used in prot.
// This is used to keep track of modified/referenced information. The access type in flags should never exceed the protection in prot.
int pmap_enter(pmap_t *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, pmap_flags_t flags);

// Removes a range of virtual to physical page mappings from the specified pmap
void pmap_remove(pmap_t *pmap, vaddr_t sva, vaddr_t eva);

// Changes the protection of all mappings in the specified range in the pmap
void pmap_protect(pmap_t *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot);

// Clears the wired attribute on the mapping for the specified virtual address
void pmap_unwire(pmap_t *pmap, vaddr_t va);

// Extracts the mapping for the specified virtual address, i.e. it gets the physical address associated with a virtual address
// Returns false if no such mapping exists
bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pa);

// Enter an unmanaged mapping for the kernel pmap. This mapping will not be affected by other systems and will always be wired and can't fail
void pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, pmap_flags_t flags);

// Removes all mappings starting at the specified virtual address to the size (in bytes) specified from the kernel pmap.
// All mappings must have been entered with pmap_kenter_pa
void pmap_kremove(vaddr_t va, size_t size);

// Copies page mappings from pmap to another
void pmap_copy(pmap_t *dst_map, pmap_t *src_map, vaddr_t dst_addr, size_t len, vaddr_t src_addr);

// Inform the pmap module that all physical mappings must now be correct. Any delayed mappings (such as TLB invalidation, address space identifier updates)
// must be completed. Should be used after calls to pmap_enter, pmap_remove, pmap_protect, pmap_kenter_pa and pmap_kremove
#define pmap_update(pmap)

// Activate the pmap, i.e. set the translation table base register with the page directory associated with the pmap
void pmap_activate(pmap_t *pmap);

// Deactivate the pmap
void pmap_deactivate(pmap_t *pmap);

// Zeros out the page at the specified physical address. The kernel may map this page into it's address space
// and must make sure that zero page is visible to any other address space (i.e. cache flush or any cache aliases)
void pmap_zero_page(paddr_t pa);

// Copies the page specified at physical address src to physical address dst. Same precautions as pmap_zero_page
void pmap_copy_page(paddr_t src, paddr_t dst);

// Lower the permissions for all mappings of vpg to prot. Used by the vmm to implement copy-on-write by setting page as read-only
// and to invalidate all mappings when prot = 0. Access permissions will never be added by this function.
void pmap_page_protect(paddr_t pa, vm_prot_t prot);

// Clear the modified attribute on the given page. Returns old value of the modified attribute
bool pmap_clear_modify(vm_page_t *page);

// Clear the referenced attribute on the given page. Returns old value of the referenced attribute
bool pmap_clear_reference(vm_page_t *page);

// Check whether modified attribute is set
#define pmap_is_modified(vpg) ((vpg)->status.modified)

// Check whether referenced attribute is set
#define pmap_is_referenced(vpg) ((vpg)->status.referenced)

#endif // __PMAP_H__
