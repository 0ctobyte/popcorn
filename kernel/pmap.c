#include <kernel/pmap.h>
#include <kernel/pmm.h>
#include <kernel/mmu.h>
#include <kernel/kassert.h>
#include <lib/asm.h>

#define IS_WITHIN_MEM_BOUNDS(addr) (((addr) >= MEMBASEADDR) && ((addr) < (MEMBASEADDR + MEMSIZE)))
#define IS_PAGE_ALIGNED(pmap, addr)  (((addr) & ((pmap).mmu.page_size - 1)) == 0)
#define ROUND_PAGE_DOWN(pmap, addr)  ((addr) & ~((pmap).mmu.page_size - 1))
#define ROUND_PAGE_UP(pmap, addr)    ((IS_PAGE_ALIGNED((pmap), (addr))) ? (addr) : (ROUND_PAGE_DOWN((pmap), (addr)) + (pmap).mmu.page_size))

// A page table has 512 page table entries when using a 4KB granule size (4096 / 8 since each PTE is 8 bytes)
// 2048 entries for a 16KB page granule and 8192 entries for a 64KB page granule
#define MAX_NUM_PTES_4KB  (512)
#define MAX_NUM_PTES_16KB (2048)
#define MAX_NUM_PTES_64KB (8192)

// The L0 table for 16KB and 64KB granule modes are a bit different since ARMv8 VMSA only supports a 48-bit VA address space
#define MAX_NUM_PTES_4KB_L0  (MAX_NUM_PTES_4KB)
#define MAX_NUM_PTES_16KB_L0 (2)
#define MAX_NUM_PTES_64KB_L0 (64)

// Macros to set and clear bits from the pte
#define PTE_SET_BIT(entry, bit)   (entry) |= (bit)
#define PTE_CLEAR_BIT(entry, bit) (entry) &= ~((bit))

// Page table entries are 8 bytes
typedef uint64_t pte_t;

typedef pte_t page_table_4KB_t[MAX_NUM_PTES_4KB];
typedef pte_t page_table_16KB_t[MAX_NUM_PTES_16KB];
typedef pte_t page_table_64KB_t[MAX_NUM_PTES_64KB];

typedef page_table_4KB_t page_table_4KB_L0_t;
typedef pte_t page_table_16KB_L0_t[MAX_NUM_PTES_16KB_L0];
typedef pte_t page_table_64KB_L0_t[MAX_NUM_PTES_64KB_L0];

// Kernel pmap
pmap_t kernel_pmap;

// Linker symbols
extern uintptr_t __kernel_virtual_start;
extern uintptr_t __kernel_physical_start;
extern uintptr_t __kernel_virtual_end;
extern uintptr_t __kernel_physical_end;

// The start of the kernel's virtual and physical address space. The start address is determined early in boot
extern paddr_t kernel_physical_start;
vaddr_t kernel_virtual_start = ((uintptr_t)(&__kernel_virtual_start));;

// These are set to the end of the kernel's virtual address space and physical address space
paddr_t kernel_physical_end;
vaddr_t kernel_virtual_end;

//vaddr_t _pmap_bootstrap_memory(size_t size) {
//    // pmap_init must be called before this function can be used, otherwise
//    // kernel_vend will be an incorrect value
//    // kernel_vend and kernel_pend should be page aligned
//    static vaddr_t placement_addr = 0;
//    placement_addr = (placement_addr == 0) ? kernel_vend : placement_addr;
//
//    vaddr_t start = placement_addr;
//    vaddr_t end = placement_addr + size;
//
//    // Allocate a new page if there is not enough memory
//    if(end >= kernel_vend) {
//        // Loop through and map the pages while incrementing kernel_pend and
//        // kernel_vend
//        for(; kernel_vend < end; kernel_vend+=PAGESIZE, kernel_pend+=PAGESIZE) {
//            pte_t entry = PTE_CREATE(kernel_pend, PTE_AP0_BIT|PTE_XN_BIT|PTE_S_BIT|PTE_TEX0_BIT|PTE_CB3);
//            KERNEL_PGTS_BASE[PGD_GET_INDEX(kernel_vend)-KERNEL_PGD_PGT_INDEX_BASE].pte[PGT_GET_INDEX(kernel_vend)] = entry;
//        }
//    }
//
//    // Zero the memory
//    memset((void*)placement_addr, 0x0, size);
//
//    placement_addr = end;
//
//    return(start);
//}

void pmap_init(void) {
    // Check for MMU supported features. We prefer 4KB pages
    if (mmu_is_4kb_granule_supported()) {
        kernel_pmap.mmu.page_size = 0x1000;
    } else if (mmu_is_16kb_granule_supported()) {
        kernel_pmap.mmu.page_size = 0x4000;
    } else if (mmu_is_64kb_granule_supported()) {
        kernel_pmap.mmu.page_size = 0x10000;
    }

    kernel_pmap.mmu.page_shift = _ctz(kernel_pmap.mmu.page_size);

    // Set the end of the kernel's virtual and physical address space
    size_t kernel_size = ROUND_PAGE_UP(kernel_pmap, kernel_virtual_end - kernel_virtual_start);
    kernel_physical_end = kernel_physical_start + kernel_size;
    kernel_virtual_end = kernel_virtual_start + kernel_size;

    // Initialize pmm
    pmm_init(MEMBASEADDR, MEMSIZE);

    // Reserve the pages used by the kernel. The PMM works on the assumption of 4KB pages so we need to make sure we reserve the correct amount of pages
    for(unsigned long i = 0; i < kernel_size >> PMM_PAGE_SHIFT; i++) {
        pmm_reserve(kernel_physical_start + (i * PMM_PAGE_SIZE));
    }

    // Finally increment the reference count on the pmap. The refcount for kernel_pmap should never be 0.
    pmap_reference(pmap_kernel());
}

//pmap_t* pmap_create(void) {
//    pmap_t *pmap = (pmap_t*)kheap_alloc(sizeof(pmap_t));
//    memset(pmap, 0, sizeof(pmap_t));
//
//    // Create pgd
//    // TODO: This will not work! We need to allocate 16 KiB of contiguous memory aligned to a 16 KiB address boundary
//    pmap->pgd = (pgd_t*)kheap_alloc(sizeof(pgd_t));
//    memset(pmap->pgd, 0, sizeof(pgd_t));
//
//    // Get the physical address of the pgd
//    pmap->pgd_pa = TRUNC_PAGE(KERNEL_PGTS_BASE[PGD_GET_INDEX((vaddr_t)pmap->pgd)-KERNEL_PGD_PGT_INDEX_BASE].pte[PGT_GET_INDEX((vaddr_t)pmap->pgd)]);
//
//    pmap_reference(pmap);
//    return pmap;
//}
//
//// TODO: Implement PMAP_CANFAIL logic
//long pmap_enter(pmap_t *pmap, vaddr_t va, paddr_t pa, vm_prot_t vm_prot, pmap_flags_t pmap_flags) {
//    // Must have a valid pmap
//    kassert(pmap != NULL && pmap->pgd != NULL && IS_WITHIN_MEM_BOUNDS(pa) && IS_PAGE_ALIGNED(pa) && IS_PAGE_ALIGNED(va));
//
//    // Encode the protection bits in the page table entry
//    // Encode the protection and pmap flags in the page table entry
//    pte_t entry = PTE_CREATE(pa, PTE_AP0_BIT | PTE_S_BIT | PTE_ENCODE_PROTECTION(vm_prot, pmap_kernel()) | PTE_ENCODE_PMAP_FLAGS(pmap_flags));
//
//    // First check if the page table for the given va exists within the page directory. If not create the page table
//    unsigned long pgd_index = PGD_GET_INDEX(va);
//    if(!PDE_PGT_EXISTS(pmap->pgd->pde[pgd_index])) {
//        // TODO: To get pa of pgt -> TRUNC_PAGE(pgt) and search kernel pgd & pgt for entry
//    }
//
//
//    kassert(entry != 0);
//    return 0;
//}
//
//void pmap_destroy(pmap_t *pmap) {
//    kassert(pmap != NULL);
//
//    atomic_dec(&pmap->refcount);
//
//    // The kernel's pmap should never be 0!! Something is fucking up
//    kassert(pmap->refcount == 0 && pmap == pmap_kernel());
//
//    // TODO: Deallocate resources for pmap if refcount is 0
//}

void pmap_reference(pmap_t *pmap) {
    // Can't be NULL!
    kassert(pmap != NULL);

    atomic_inc(&pmap->refcount);
}

void pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp) {
    if(vstartp != NULL) *vstartp = kernel_virtual_start;
    // FIXME This should be the max virtual end address of kernel, not it's current virtual end
    if(vendp != NULL) *vendp = kernel_virtual_end;
}

//// TODO: Should a lock be used to access kernel_pmap?
//void pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t vm_prot, pmap_flags_t pmap_flags) {
//    // The mapping must be in the kernel virtual address space
//    kassert(va >= (uintptr_t)(&__kernel_virtual_start) && IS_PAGE_ALIGNED(pa) && IS_PAGE_ALIGNED(va));
//
//    // Encode the protection and pmap flags in the page table entry
//    pte_t entry = PTE_CREATE(pa, PTE_AP0_BIT | PTE_S_BIT | PTE_ENCODE_PROTECTION(vm_prot, pmap_kernel()) | PTE_ENCODE_PMAP_FLAGS(pmap_flags));
//
//
//    // Now we must place the page table entry in the correct kernel page table
//    // Since we know that the pgts are laid out contiguously in memory we can cheat by
//    // accessing the correct pgt directly without having to loop over the pmap_kernel's pgt_entries
//    // to search for the pgt_entry with the correct offset in the pgd.
//
//    // Place the entry in the page table if one doesn't already exist
//    pte_t existing_entry = KERNEL_PGTS_BASE[PGD_GET_INDEX(va)-KERNEL_PGD_PGT_INDEX_BASE].pte[PGT_GET_INDEX(va)];
//    kassert(!(existing_entry & PTE_PAGE_BIT));
//    KERNEL_PGTS_BASE[PGD_GET_INDEX(va)-KERNEL_PGD_PGT_INDEX_BASE].pte[PGT_GET_INDEX(va)] = entry;
//
//    // Update the stats
//    pmap_kernel()->pmap_stats.wired_count++;
//    pmap_kernel()->pmap_stats.resident_count++;
//}

vaddr_t pmap_steal_memory(size_t vsize) {
    // pmap_init must be called before this function can be used, otherwise
    // kernel_vend will be an incorrect value
    // kernel_vend and kernel_pend should be page aligned
    // This function should only be used before pmm_init is called
    //static vaddr_t placement_addr = 0;
    //placement_addr = (placement_addr == 0) ? kernel_virtual_end : placement_addr;

    //// Make sure enough memory is left!
    //kassert((UINT32_MAX-placement_addr) >= vsize);

    //vaddr_t start = placement_addr;
    //vaddr_t end = placement_addr + vsize;

    //// Allocate a new page if there is not enough memory
    //if(end >= kernel_vend) {
    //    // Loop through and map the pages using pmap_kenter_pa while incrementing kernel_pend and kernel_vend
    //    for(; kernel_vend < end; kernel_vend+=PAGESIZE, kernel_pend+=PAGESIZE) {
    //        pmap_kenter_pa(kernel_vend, kernel_pend,  VM_PROT_DEFAULT, PMAP_WRITE_BACK);
    //    }
    //}

    //// Zero the memory
    //memset((void*)placement_addr, 0x0, vsize);

    //placement_addr = end;

    return 0;
}
