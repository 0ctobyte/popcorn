#include <kernel/pmap.h>
#include <kernel/pmm.h>
#include <kernel/mmu.h>
#include <kernel/barrier.h>
#include <kernel/cache.h>
#include <kernel/kassert.h>
#include <lib/asm.h>
#include <string.h>

#define _4KB   (0x1000)
#define _16KB  (0x4000)
#define _64KB  (0x10000)

#define IS_WITHIN_MEM_BOUNDS(addr) (((addr) >= MEMBASEADDR) && ((addr) < (MEMBASEADDR + MEMSIZE)))

// Lower level page table have 512 page table entries when using a 4KB granule size (4096 / 8 since each PTE is 8 bytes)
// 2048 entries for a 16KB page granule and 8192 entries for a 64KB page granule
#define MAX_NUM_PTES_LL_4KB  (512)
#define MAX_NUM_PTES_LL_16KB (2048)
#define MAX_NUM_PTES_LL_64KB (8192)

// The base translation table for 16KB and 64KB granule modes are a bit different since ARMv8 VMSA only supports a 48-bit VA address space
#define MAX_NUM_PTES_TTB_4KB  (MAX_NUM_PTES_LL_4KB)
#define MAX_NUM_PTES_TTB_16KB (2)
#define MAX_NUM_PTES_TTB_64KB (64)

#define GET_TABLE_IDX_WIDTH(page_shift)                 ((page_shift) - 3l)
#define GET_TABLE_IDX_MASK(idx_width)                   ((1l << (width)) - 1l)
#define GET_TABLE_IDX_LSB(level, idx_width, page_shift) ((page_shift) + ((3l-(level)) * (idx_width)))
#define GET_TABLE_IDX(va, lsb, mask)                    (((va) >> (lsb)) & (mask))
#define GET_TABLE_IDX_AT_LEVEL(va, level, page_shift)\
({\
    vaddr_t vaddr = (va) & ~(0xFFFF000000000000);\
    unsigned int width = GET_TABLE_IDX_WIDTH(page_shift);\
    unsigned int mask = GET_TABLE_IDX_MASK(width);\
    unsigned int lsb = GET_TABLE_IDX_LSB(level, width, page_shift);\
    GET_TABLE_IDX(va, lsb, mask);\
})
#define GET_TABLE_IDX_WIDTH_TTB(page_shift)\
({\
    unsigned int width = GET_TABLE_IDX_WIDTH(page_shift);\
    unsigned int lsb = GET_TABLE_IDX_LSB(page_shift == 16 ? 1 : 0, width, page_shift);\
    (lsb + width) < 48 ? width : (48 - lsb);\
})
#define GET_TTB_VA(page_shift)  ((-1l << (page_shift)) & ~0xFFFF000000000000)
#define GET_TABLE_VA_BASE(pmap) ((-1l << ((pmap).page_shift + ((3l - (((pmap).page_size == _64KB) ? 1l : 0l)) * ((pmap).page_shift - 3l)))) & ((pmap).is_kernel ? -1l : ~0xFFFF000000000000))

#define MAX_NUM_PTES_TTB(page_shift) (1 << GET_TABLE_IDX_WIDTH_TTB(page_shift))
#define MAX_NUM_PTES_LL(page_size)   ((page_size) >> 3)

#define ROUND_PAGE_DOWN(page_size, addr)  ((long)(addr) & ~((long)((page_size) - 1l)))
#define ROUND_PAGE_UP(page_size, addr)    ((IS_PAGE_ALIGNED((page_size), (addr))) ? (long)(addr) : (ROUND_PAGE_DOWN((page_size), (addr)) + (long)(page_size)))

#define BLOCK_SIZE(page_shift)            (1l << ((page_shift) + (page_shift) - 3l))
#define IS_PAGE_ALIGNED(page_size, addr)  (((long)(addr) & ((long)(page_size) - 1l)) == 0)
#define IS_BLOCK_ALIGNED(page_size, addr) (((long)(addr) & (BLOCK_SIZE(_ctz(page_size)) - 1l)) == 0)

// Upper attributes for page table descriptors

// NSTable values
typedef enum {
    T_SECURE     = 0,                  // Tables with this attribute are in the secure PA space. Subsequent levels of lookup keep their NS or NSTable meanings
    T_NON_SECURE = 0x8000000000000000, // Tables with this attribute are in the non-secure PA space. As a results subsequent levels of lookups are also forced non-secure
} t_ns_attr_t;

// APTable values
typedef enum {
    T_AP_NONE      = 0,                  // No effect on permissions on lower levels of lookups
    T_AP_NO_EL0    = 0x2000000000000000, // Access at EL0 not permitted, regardless of permissions at lower levels of lookup
    T_AP_RO        = 0x4000000000000000, // Write access not permitted at any exception level regardless of permissions at lower levels of lookup
    T_AP_RO_NO_EL0 = 0x6000000000000000, // Write access not permitted at any exception level and read access not permitted at EL0 regardless of permissions at lower levels of lookup
} t_ap_attr_t;

// UXNTable values
typedef enum {
    T_NON_UXN = 0,                  // No effect on subsequent levels of lookups
    T_UXN     = 0x1000000000000000, // Sets the unprivileged execute-never attribute at EL0 which applies to all subsequent levels of lookups
} t_uxn_attr_t;

// PXNTable values
typedef enum {
    T_NON_PXN = 0,                  // No effect on subsequent levels of lookups
    T_PXN     = 0x0800000000000000, // Sets the execute-never attribute at EL1 or higher exception levels and applies to all subsequent levels of lookups
} t_pxn_attr_t;

typedef struct {
    t_ns_attr_t ns;
    t_ap_attr_t ap;
    t_uxn_attr_t uxn;
    t_pxn_attr_t pxn;
} t_attr_t;

#define T_ATTR(t_attr) ((t_attr).ns | (t_attr).ap | (t_attr).uxn | (t_attr).pxn)
#define T_ATTR_EXTRACT(pte)\
((t_attr_t){\
    .ns = (t_ns_attr_t)((pte) & T_NON_SECURE),\
    .ap = (t_ap_attr_t)((pte) & T_AP_RO_NO_EL0),\
    .uxn = (t_uxn_attr_t)((pte) & T_UXN),\
    .pxn = (t_pxn_attr_t)((pte) & T_PXN)\
})

// Upper attributes for page and block descriptors

// UXN values
typedef enum {
    BP_NON_UXN = 0,                  // Allow instruction execution at EL0 for this memory region
    BP_UXN     = 0x0040000000000000, // Execute-never for EL0 exception level
} bp_uxn_attr_t;

// PXN values
typedef enum {
    BP_NON_PXN = 0,                  // Allow instruction execution at higher exception levels for this memory region
    BP_PXN     = 0x0020000000000000, // Execute=never for higher exception levels
} bp_pxn_attr_t;

// Contiguous bit
typedef enum {
    BP_NON_CONTIGUOUS = 0,                  // Don't set contiguous bit
    BP_CONTIGUOUS     = 0x0010000000000000, // Sets the contiguous which can be used to hint the MMU hardware that this entry is part of a contiguous set of entries that the TLB can cache in a single TLB entry
} bp_contiguous_attr_t;

// Lower attributes for page and block descriptors

// non-global bit
typedef enum {
    BP_GLOBAL     = 0,     // TLB entries for this page apply to all ASID values
    BP_NON_GLOBAL = 0x800, // TLB entries for this page apply only to the current ASID value
} bp_ng_attr_t;

// Access flag
typedef enum {
    BP_NO_AF = 0,     // Access flag not set
    BP_AF    = 0x400, // Set the access flag
} bp_af_attr_t;

// Shareability values
typedef enum {
    BP_NSH = 0,     // Non shareable. Data in this memory region is not required to be coherent to outside agents
    BP_OSH = 0x200, // Outer shareable. Data in this memory region is required to be coherent to agents in the outer shareable domain
    BP_ISH = 0x300, // Inner shareable. Data in this memory region is required to be coherent to agents in the inner shareable domain
} bp_sh_attr_t;

// Access permission values
typedef enum {
    BP_AP_RW_NO_EL0 = 0,    // R/W access from higher exception levels
    BP_AP_RW        = 0x40, // R/W access from any exception level
    BP_AP_RO_NO_EL0 = 0x80, // Read only access from higher exception levels
    BP_AP_RO        = 0xc0, // Read only access from any exception level
} bp_ap_attr_t;

// non-secure bit
typedef enum {
    BP_SECURE     = 0,    // Memory region is in the secure PA region
    BP_NON_SECURE = 0x20, // Memory region is in the non-secure PA region
} bp_ns_attr_t;

// Memory attribute index
typedef enum {
    BP_MA_DEVICE_NGNRNE = 0,    // Device nGnRnE, non-gathering, non-reordering, non-early write acknowledgment. Basically strongly ordered
    BP_MA_DEVICE_NGNRE  = 0x4,  // Device nGnRE, non-gathering, non-reordering with early write acknowledgement. Can be used with PCIE endpoints that support posted writes
    BP_MA_NORMAL_NC     = 0x8,  // Outer & inner non-cacheable. Outer cacheability typically implies the last level cache
    BP_MA_NORMAL_INC    = 0xc,  // Inner non-cacheable only
    BP_MA_NORMAL_WBWARA = 0x10, // Outer & inner write-back/write-allocate/read-allocate
    BP_MA_NORMAL_WTWARA = 0x14, // Outer & inner write-through/write-allocate/read-allocate
    BP_MA_NORMAL_WTWNRA = 0x18, // Outer & inner write-through/no write-allocate/read-allocate
    BP_MA_NORMAL_WTWNRN = 0x1c, // Outer & inner write-through/no write-allocate/no read-allocate
} bp_ma_attr_t;

typedef struct {
    bp_uxn_attr_t uxn;
    bp_pxn_attr_t pxn;
    bp_contiguous_attr_t contiguous;
} bp_uattr_t;

typedef struct {
    bp_ng_attr_t ng;
    bp_af_attr_t af;
    bp_sh_attr_t sh;
    bp_ap_attr_t ap;
    bp_ns_attr_t ns;
    bp_ma_attr_t ma;
} bp_lattr_t;

#define BP_UATTR(bp_uattr) ((bp_uattr).uxn | (bp_uattr).pxn | (bp_uattr).contiguous)
#define BP_UATTR_EXTRACT(pte)\
((bp_uattr_t){\
    .uxn = (bp_uxn_attr_t)((pte) & BP_UXN),\
    .pxn = (bp_pxn_attr_t)((pte) & BP_PXN),\
    .contiguous = (bp_contiguous_attr_t)((pte) & BP_CONTIGUOUS),\
})
#define BP_LATTR(bp_lattr) ((bp_lattr).ng | (bp_lattr).af | (bp_lattr).af | (bp_lattr).sh | (bp_lattr).ap | (bp_lattr).ns | (bp_lattr).ma)
#define BP_LATTR_EXTRACT(pte)\
((bp_lattr_t){\
    .ng = (bp_ng_attr_t)((pte) & BP_NON_GLOBAL),\
    .af = (bp_af_attr_t)((pte) & BP_AF),\
    .sh = (bp_sh_attr_t)((pte) & BP_ISH),\
    .ap = (bp_ap_attr_t)((pte) & BP_AP_RO),\
    .ns = (bp_ns_attr_t)((pte) & BP_NON_SECURE),\
    .ma = (bp_ma_attr_t)((pte) & BP_MA_NORMAL_WTWNRN)\
})

#define PTE_TO_PA(page_size, pte)  (((pte) & 0xffffffffffff) & -(page_size))
#define PA_TO_PTE(page_size, pa) PTE_TO_PA(page_size, pa)
#define MAKE_TDE(pmap, pa, t_attr, bp_lattr)   (PA_TO_PTE((pmap).page_size, pa) | T_ATTR(t_attr) | BP_LATTR(bp_lattr) | 0x3)
#define MAKE_BDE(pmap, pa, bp_uattr, bp_lattr) (PA_TO_PTE((pmap).page_size, pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x1)
#define MAKE_PDE(pmap, pa, bp_uattr, bp_lattr) (PA_TO_PTE((pmap).page_size, pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x3)

#define IS_PTE_VALID(pte) ((pte) & 0x1)
#define IS_PDE_VALID(pte) (((pte) & 0x3) == 0x3)
#define IS_TDE_VALID(pte) IS_PDE_VALID(pte)
#define IS_BDE_VALID(pte) (((pte) & 0x3) == 0x1)

typedef enum {
    MA_DEVICE_NGNRNE = 0x00, // Device non-gathering, non-reordering, no early-write acknowledgement
    MA_DEVICE_NGNRE  = 0x04, // Device non-gathering, non-reordering, early-write acknowledgement
    MA_NORMAL_NC     = 0x44, // Outer & inner non-cacheable. Outer cacheability typically implies the last level cache
    MA_NORMAL_INC    = 0xf4, // Inner non-cacheable only
    MA_NORMAL_WBWARA = 0xff, // Outer & inner write-back/write-allocate/read-allocate
    MA_NORMAL_WTWARA = 0xbb, // Outer & inner write-through/write-allocate/read-allocate
    MA_NORMAL_WTWNRA = 0xaa, // Outer & inner write-through/no write-allocate/read-allocate
    MA_NORMAL_WTWNRN = 0x88, // Outer & inner write-through/no write-allocate/no read-allocate
} ma_t;

// Representation of the MAIR register
typedef struct {
    ma_t attrs[8];
} ma_index_t;

// Convert an ma_index_t to an 8 byte value that can be programmed into the MAIR
#define MAIR(mair) ((long)(mair).attrs[0] | ((long)(mair).attrs[1] << 8) | ((long)(mair).attrs[2] << 16) | ((long)(mair).attrs[3] << 24) | ((long)(mair).attrs[4] << 32) | ((long)(mair).attrs[5] << 40) | ((long)(mair).attrs[6] << 48) | ((long)(mair).attrs[7] << 56))

// Page table entries are 8 bytes
typedef uint64_t pte_t;

// Kernel pmap
pmap_t kernel_pmap;

// Linker symbols
extern uintptr_t __kernel_virtual_start;
extern uintptr_t __kernel_physical_start;
extern uintptr_t __kernel_virtual_end;
extern uintptr_t __kernel_physical_end;

// The start of the kernel's virtual and physical address space. The start address is determined early in boot
extern paddr_t kernel_physical_start;
vaddr_t kernel_virtual_start = ((uintptr_t)(&__kernel_virtual_start));

// These are set to the end of the kernel's virtual address space and physical address space
paddr_t kernel_physical_end = 0;
vaddr_t kernel_virtual_end = 0;

bool _pmap_alloc_table(pmap_t *pmap, pte_t *table_pte) {
    // Attributes for the table descriptors
    t_attr_t t_attr_table = (t_attr_t){.ns = T_NON_SECURE, .ap = T_AP_NO_EL0, .uxn = T_UXN, .pxn = T_NON_PXN};
    bp_lattr_t bp_lattr_table = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    // Allocate a page for this new table and zero it out
    pte_t new_table;
    if (!pmm_alloc((paddr_t*)&new_table)) return false;
    memset((void*)new_table, 0, pmap->page_size);

    // Make a table descriptor entry
    *table_pte = MAKE_TDE(*pmap, (paddr_t)new_table, t_attr_table, bp_lattr_table);
    return true;
}

void _pmap_update_pte(vaddr_t va, pte_t *old_pte, pte_t new_pte) {
    // Use break-before-make rule if the old PTE was valid. We must do this in the following cases:
    // - Changing memory type
    // - Changing cacheability
    // - Changing output address
    // - Changing block size in either direction (page to block or block to page)
    // - Creating a global entry from a non-global entry
    if (!IS_PTE_VALID(*old_pte)) {
        *old_pte = new_pte;
        return;
    }

    // The break-before-make procedure:
    // 1. Replace old PTE with invalid entry and issue DSB
    *old_pte = 0;
    barrier_dsb();

    // 2. Invalidate the TLB entry by VA
    tlb_invalidate(va);

    // 3. Write new pte and issue DSB
    *old_pte = new_pte;
    barrier_dsb();
}

void _pmap_clear_pte(vaddr_t va, pte_t *old_pte) {
    if (!IS_PTE_VALID(*old_pte)) {
        *old_pte = 0;
        return;
    }

    *old_pte = 0;
    barrier_dsb();
    tlb_invalidate(va);
}

bool _pmap_map_range(pmap_t *pmap, vaddr_t vaddr, paddr_t paddr, size_t size, bp_uattr_t bpu, bp_lattr_t bpl) {
    kassert(IS_PAGE_ALIGNED(pmap->page_size, vaddr) && IS_PAGE_ALIGNED(pmap->page_size, paddr));

    unsigned long block_size = BLOCK_SIZE(pmap->page_shift);
    unsigned long max_num_ptes_ll = MAX_NUM_PTES_LL(pmap->page_size);

    // Get rid of extraneous VA bits not used in translation
    vaddr_t va = vaddr & ~(0xFFFF000000000000);
    paddr_t pa = paddr;

    for (unsigned long page_offset = 0, block_size = BLOCK_SIZE(pmap->page_shift); page_offset < size;) {
        pte_t *table = (pte_t*)pmap->ttb;

        // Walk the page tables checking if a table is missing at an intermediate level and if so allocate one
        unsigned int level = pmap->page_size == _64KB ? 1 : 0;
        unsigned int width = GET_TABLE_IDX_WIDTH(pmap->page_shift), mask = GET_TABLE_IDX_MASK(width), lsb = GET_TABLE_IDX_LSB(level, width, pmap->page_shift);
        for (; level < 3; level++, lsb -= width) {
            unsigned int index = GET_TABLE_IDX(va, lsb, mask);
            pte_t pte = table[index];

            if (!IS_TDE_VALID(pte)) {
                // Check if we can make a block entry at level 2. We need to make sure the PA is block aligned and we have enough to map
                if (level == 2 && IS_BLOCK_ALIGNED(pmap->page_size, pa) && (size - page_offset) > block_size) break;

                // Allocate a table if it doesn't exist at the current level
                if (!_pmap_alloc_table(pmap, &pte)) return false;
                _pmap_update_pte(va, &table[index], pte);
            }

            // Update table pointer to the next table
            table = (pte_t*)PTE_TO_PA(pmap->page_size, pte);
        }

        // Now map pages or blocks in a loop until we either run out of room in the table, completely mapped the specified size, or a block mapping needs to be broken down into page mappings
        // In those cases, start the table walk again to get the next table pointer
        size_t mapped_size = (level == 2) ? block_size : pmap->page_size;
        for (unsigned int index = GET_TABLE_IDX(va, lsb, mask); (page_offset < size) && (index < max_num_ptes_ll) && ((level == 2) ? ((size - page_offset) > block_size) : true); index++) {
            _pmap_update_pte(va, &table[index], (level == 2) ? MAKE_BDE(*pmap, pa, bpu, bpl) : MAKE_PDE(*pmap, pa, bpu, bpl));
            page_offset += mapped_size;
            pa += mapped_size;
            va += mapped_size;
        }
    }

    return true;
}

bool _pmap_unmap_range(pmap_t *pmap, vaddr_t vaddr, size_t size) {
    kassert(IS_PAGE_ALIGNED(pmap->page_size, vaddr));

    unsigned long block_size = BLOCK_SIZE(pmap->page_shift);
    unsigned long block_mask = block_size - 1;
    unsigned long max_num_ptes_ll = MAX_NUM_PTES_LL(pmap->page_size);

    // Get rid of extraneous VA bits not used in translation
    vaddr_t va = vaddr & ~(0xFFFF000000000000);

    for (unsigned long page_offset = 0, block_size = BLOCK_SIZE(pmap->page_shift); page_offset < size;) {
        pte_t *tables[4] = {NULL, NULL, NULL, NULL};
        pte_t *table = (pte_t*)pmap->ttb;

        // Walk the page tables. If a page table does not exist where it should then the supplied VA range is too large for whatever reason
        unsigned int level = pmap->page_size == _64KB ? 1 : 0;
        unsigned int width = GET_TABLE_IDX_WIDTH(pmap->page_shift), mask = GET_TABLE_IDX_MASK(width), lsb = GET_TABLE_IDX_LSB(level, width, pmap->page_shift);
        for (; level < 3; level++, lsb -= width) {
            unsigned int index = GET_TABLE_IDX(va, lsb, mask);
            pte_t pte = table[index];

            // Check if there is a block entry in the way that is not going to be completely unmapped
            // If so, unmap the whole block and then map it as pages
            if(level == 2 && IS_BDE_VALID(pte)) {
                size_t remaining_size = size - page_offset;
                if (remaining_size > block_size && IS_BLOCK_ALIGNED(pmap->page_size, va)) break;

                bp_uattr_t bpu = BP_UATTR_EXTRACT(pte);
                bp_lattr_t bpl = BP_LATTR_EXTRACT(pte);

                vaddr_t va_block = va & ~block_mask;
                paddr_t pa = PTE_TO_PA(pmap->page_size, pte);
                size_t va_size = block_size >> 1;

                _pmap_clear_pte(va_block, &table[index]);
                _pmap_map_range(pmap, va_block, pa, va_size, bpu, bpl);
                _pmap_map_range(pmap, va_block + va_size, pa + va_size, va_size, bpu, bpl);

                // This should now hold a table descriptor
                pte = table[index];
            }

            // We expect there to be a page table
            if (!IS_TDE_VALID(pte)) return false;

            // Update table pointer to the next table
            tables[level] = &table[index];
            table = (pte_t*)PTE_TO_PA(pmap->page_size, pte);
        }

        // Now unmap pages or blocks in a loop until we either run out of room in the table, completely unmapped the specified size, or a block unmapping needs to be broken down into page unmappings
        // In those cases, start the table walk again to get the next table pointer
        size_t mapped_size = (level == 2) ? block_size : pmap->page_size;
        for (unsigned int index = GET_TABLE_IDX(va, lsb, mask); (page_offset < size) && (index < max_num_ptes_ll) && ((level == 2) ? ((size - page_offset) > block_size) : true); index++) {
            _pmap_clear_pte(va, &table[index]);
            page_offset += mapped_size;
            va += mapped_size;
        }

        // Scan the page tables we just walked and free any that are completely empty
        tables[level] = table;
        for (int i = level; i >= 0 && tables[i] != NULL; i--) {
            pte_t *table_base = (pte_t*)((uintptr_t)tables[i] & ~mask);

            bool empty = true;
            for (int j = 0; j < max_num_ptes_ll; j++) {
                if (IS_PTE_VALID(table_base[j])) {
                    empty = false;
                    break;
                }
            }

            // Don't check the other tables if this one is not empty
            if (!empty) break;

            // If it is empty then clear the parent table PTE and free the page
            if (i != 0) {
                pmm_free(PTE_TO_PA(pmap->page_size, *tables[i - 1]));
                _pmap_clear_pte(vaddr, tables[i - 1]);
            }
        }
    }

    return true;
}

void pmap_init(void) {
    // Check for MMU supported features. We prefer 4KB pages
    kernel_pmap.page_size = mmu_is_4kb_granule_supported() ? _4KB : (mmu_is_16kb_granule_supported() ? _16KB : _64KB);

    kernel_pmap.page_shift = _ctz(kernel_pmap.page_size);
    kernel_pmap.is_kernel = true;

    // Set the start and end of the kernel's virtual and physical address space
    // kernel_virtual_end is set to 0 at boot so that early bootstrap allocaters will kassert if they are called before pmap_init
    size_t kernel_size = (uintptr_t)(&__kernel_virtual_end) - (uintptr_t)(&__kernel_virtual_start);
    kernel_physical_end = ROUND_PAGE_UP(kernel_pmap.page_size, kernel_physical_start + kernel_size);
    kernel_virtual_end = ROUND_PAGE_UP(kernel_pmap.page_size, kernel_virtual_start + kernel_size);

    // pmm_init won't have any way to allocate memory this early in the boot process so pre-allocate memory for it
    size_t pmm_size = ROUND_PAGE_UP(kernel_pmap.page_size, pmm_get_size_requirement(MEMSIZE, kernel_pmap.page_size));
    vaddr_t pmm_va = kernel_virtual_end;
    pmm_init(kernel_physical_end, MEMBASEADDR, MEMSIZE, kernel_pmap.page_size);
    kernel_physical_end += pmm_size;
    kernel_virtual_end += pmm_size;
    kernel_size = kernel_virtual_end - kernel_virtual_start;

    // Reserve the physical pages used by the kernel
    for(unsigned long i = 0, num_pages = kernel_size >> kernel_pmap.page_shift; i < num_pages; i++) {
        pmm_reserve(kernel_physical_start + (i << kernel_pmap.page_shift));
    }

    // Let's grab a page for the base translation table
    // The base table for 16KB granule only has 2 entries while the 64KB granule base table only has 64 entries.
    kassert(pmm_alloc(&kernel_pmap.ttb));
    memset((void*)kernel_pmap.ttb, 0, kernel_pmap.page_size);

    size_t max_num_ptes_ttb = MAX_NUM_PTES_TTB(kernel_pmap.page_shift);
    size_t max_num_ptes_ll = MAX_NUM_PTES_LL(kernel_pmap.page_size);
    pte_t *ttb = (pte_t*)kernel_pmap.ttb;

    // Attributes for the kernel's table descriptors
    t_attr_t t_attr_table = (t_attr_t){.ns = T_NON_SECURE, .ap = T_AP_NO_EL0, .uxn = T_UXN, .pxn = T_NON_PXN};
    bp_lattr_t bp_lattr_table = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    // Point the last entry in the TTB to itself. For 16KB and 64KB page granules we need to fill the rest of the TTB page with the last entry
    // This recursive page mapping allows us to access any page table in a fixed VA aperture
    ttb[max_num_ptes_ttb - 1] = MAKE_TDE(kernel_pmap, kernel_pmap.ttb, t_attr_table, bp_lattr_table);
    if (kernel_pmap.page_size == _16KB || kernel_pmap.page_size == _64KB) {
        pte_t last_ttb_pte = ttb[max_num_ptes_ttb - 1];
        for(unsigned long idx = max_num_ptes_ttb; idx < max_num_ptes_ll; idx++) {
            ttb[idx] = last_ttb_pte;
        }
    }

    // Attributes for the Kernel's page table mappings
    bp_uattr_t bp_uattr_page = (bp_uattr_t){.uxn = BP_UXN, .pxn = BP_NON_PXN, .contiguous = BP_NON_CONTIGUOUS};
    bp_lattr_t bp_lattr_page = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    // Map the kernel's virtual address space to it's physical location
    kassert(_pmap_map_range(&kernel_pmap, kernel_virtual_start, kernel_physical_start, kernel_size, bp_uattr_page, bp_lattr_page));

    // FIXME: temporary mapping of UART
    extern uintptr_t uart_base_addr;
    bp_uattr_t bp_uattr_dev = (bp_uattr_t){.uxn = BP_UXN, .pxn = BP_PXN, .contiguous = BP_NON_CONTIGUOUS};
    bp_lattr_t bp_lattr_dev = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_DEVICE_NGNRNE};
    vaddr_t kernel_devices_virtual_start = 0xFFFFFC0000000000;
    kassert(_pmap_map_range(&kernel_pmap, kernel_devices_virtual_start, uart_base_addr, kernel_pmap.page_size, bp_uattr_dev, bp_lattr_dev));
    uart_base_addr = kernel_devices_virtual_start;
    kernel_devices_virtual_start += kernel_pmap.page_size;

    // Now let's create temporary mappings to identity map the kernel's physical address space (needed when we enable the MMU)
    // We need to allocate a new TTB since these mappings will be in TTBR0 while the kernel virtual mappings are in TTBR1
    paddr_t ttb0, ttb1 = kernel_pmap.ttb;
    kassert(pmm_alloc(&ttb0));
    kernel_pmap.ttb = ttb0;

    kassert(_pmap_map_range(&kernel_pmap, kernel_physical_start, kernel_physical_start, kernel_size, bp_uattr_page, bp_lattr_page));

    // Restore TTBR1 in the kernel's pmap and Create MAIR
    kernel_pmap.ttb = ttb1;
    ma_index_t ma_index = {.attrs = {MA_DEVICE_NGNRNE, MA_DEVICE_NGNRE, MA_NORMAL_NC, MA_NORMAL_INC, MA_NORMAL_WBWARA, MA_NORMAL_WTWARA, MA_NORMAL_WTWNRA, MA_NORMAL_WTWNRN}};

    // Finally enable the MMU!
    mmu_enable(ttb0, ttb1, MAIR(ma_index), kernel_pmap.page_size);
    mmu_kernel_longjmp(kernel_physical_start, kernel_virtual_start);

    // Tell pmm to switch to using the virtual address space to access it's data structures
    pmm_set_va(pmm_va);

    // Finally increment the reference count on the pmap. The refcount for kernel_pmap should never be 0.
    pmap_reference(pmap_kernel());
}

void pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp) {
    // The kernel's virtual address space ends where the page table VA space starts
    if(vstartp != NULL) *vstartp = kernel_virtual_start;
    if(vendp != NULL) *vendp = GET_TABLE_VA_BASE(kernel_pmap);
}

vaddr_t pmap_steal_memory(size_t vsize) {
    kassert(kernel_virtual_end != 0);

    static vaddr_t placement_addr = 0;
    placement_addr = (placement_addr == 0) ? kernel_virtual_end : placement_addr;

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
