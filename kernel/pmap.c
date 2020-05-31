#include <kernel/pmap.h>
#include <kernel/mmu.h>
#include <kernel/barrier.h>
#include <kernel/kassert.h>
#include <lib/asm.h>
#include <string.h>

#define _4KB   (0x1000)
#define _16KB  (0x4000)
#define _64KB  (0x10000)

#define IS_WITHIN_MEM_BOUNDS(addr) (((addr) >= MEMBASEADDR) && ((addr) < (MEMBASEADDR + MEMSIZE)))

#define MAX_NUM_PTES_TTB                  (1 << (48 - (PAGESHIFT + ((3l - (PAGESHIFT == 16 ? 1 : 0)) * (PAGESHIFT - 3l)))))
#define MAX_NUM_PTES_LL                   (PAGESIZE >> 3)

#define ROUND_PAGE_DOWN(addr)             ((long)(addr) & ~((long)((PAGESIZE) - 1l)))
#define ROUND_PAGE_UP(addr)               (IS_PAGE_ALIGNED(addr) ? (long)(addr) : ROUND_PAGE_DOWN(addr) + (long)(PAGESIZE))

#define BLOCK_SIZE                        (1l << (PAGESHIFT + PAGESHIFT - 3l))
#define IS_PAGE_ALIGNED(addr)             (((long)(addr) & ((long)(PAGESIZE) - 1l)) == 0)
#define IS_BLOCK_ALIGNED(addr)            (((long)(addr) & (BLOCK_SIZE - 1l)) == 0)

#define GET_TTB_VA(pmap)                  ((-1l << PAGESHIFT) & ((pmap) == pmap_kernel() ? -1l : ~0xFFFF000000000000))
#define GET_TABLE_VA_BASE(pmap)           ((-1l << (PAGESHIFT + ((3l - ((PAGESIZE == _64KB) ? 1l : 0l)) * (PAGESHIFT - 3l)))) & ((pmap) == pmap_kernel() ? -1l : ~0xFFFF000000000000))
#define GET_TABLE_VA(pmap, parent_table_va, index, width) ((((parent_table_va) << (width)) | ((index) << PAGESHIFT)) & ((pmap) == pmap_kernel() ? -1 : ~0xFFFF000000000000))

#define TEMP_PAGE_VA0                     (GET_TABLE_VA_BASE(pmap_kernel()) - (PAGESIZE << 2))
#define TEMP_PAGE_VA1                     (TEMP_PAGE_VA0 + PAGESIZE)
#define TEMP_PAGE_VA2                     (TEMP_PAGE_VA1 + PAGESIZE)
#define TEMP_PAGE_VA3                     (TEMP_PAGE_VA2 + PAGESIZE)
#define TEMP_PAGE_VA(level)               (level == 0 ? TEMP_PAGE_VA0 : (level == 1 ? TEMP_PAGE_VA1 : (level == 2 ? TEMP_PAGE_VA2 : TEMP_PAGE_VA3)))

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

#define PTE_TO_PA(pte)                         (((pte) & 0xffffffffffff) & -(PAGESIZE))
#define PA_TO_PTE(pa)                          PTE_TO_PA(pa)
#define MAKE_TDE(pa, t_attr, bp_lattr)         (PA_TO_PTE(pa) | T_ATTR(t_attr) | BP_LATTR(bp_lattr) | 0x3)
#define MAKE_BDE(pa, bp_uattr, bp_lattr)       (PA_TO_PTE(pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x1)
#define MAKE_PDE(pa, bp_uattr, bp_lattr)       (PA_TO_PTE(pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x3)

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
extern paddr_t kernel_physical_end;
vaddr_t kernel_virtual_end = 0;

unsigned long PAGESIZE;
unsigned long PAGESHIFT;

void _pmap_enter(pmap_t *pmap, vaddr_t vaddr, paddr_t paddr, bp_uattr_t bpu, bp_lattr_t bpl);
void _pmap_map_temp_page(vaddr_t temp_va, paddr_t paddr);

void _pmap_setup_table_recursive_mapping(pmap_t *pmap) {
    size_t max_num_ptes_ttb = MAX_NUM_PTES_TTB;
    size_t max_num_ptes_ll = MAX_NUM_PTES_LL;
    pte_t *ttb = (pte_t*)pmap->ttb;

    // Attributes for the table descriptors
    t_attr_t t_attr_table = (t_attr_t){.ns = T_NON_SECURE, .ap = T_AP_NO_EL0, .uxn = T_UXN, .pxn = T_NON_PXN};
    bp_lattr_t bp_lattr_table = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    // Point the last entry in the TTB to itself. For 16KB and 64KB page granules we need to fill the rest of the TTB page with the last entry
    // This recursive page mapping allows us to access any page table in a fixed VA aperture
    ttb[max_num_ptes_ttb - 1] = MAKE_TDE(pmap->ttb, t_attr_table, bp_lattr_table);
    if (PAGESIZE == _16KB || PAGESIZE == _64KB) {
        pte_t last_ttb_pte = ttb[max_num_ptes_ttb - 1];
        for(unsigned long idx = max_num_ptes_ttb; idx < max_num_ptes_ll; idx++) {
            ttb[idx] = last_ttb_pte;
        }
    }
}

pte_t _pmap_alloc_table(pmap_t *pmap) {
    // Allocate a page for this new table and zero it out. Table attributes are relaxed as we want per-page attributes to take precendent on access permissions
    // Table descriptors don't have lower-page attributes but they are needed for the recursive page table mapping which allows us to access a table in VA as if it was a page
    paddr_t new_table;
    bp_lattr_t bpl_table = (bp_lattr_t){.ng = pmap == pmap_kernel() ? BP_GLOBAL : BP_NON_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};
    t_attr_t ta = (t_attr_t){.ns = T_NON_SECURE, .ap = T_AP_NONE, .uxn = T_NON_UXN, .pxn = T_NON_PXN};

    vm_page_t *page = vm_page_alloc();
    kassert(page != NULL);
    page->status.wired_count++;

    new_table = vm_page_to_pa(page);
    return MAKE_TDE(new_table, ta, bpl_table);
}

void _pmap_map_temp_page(vaddr_t temp_va, paddr_t paddr) {
    bp_uattr_t bpu = (bp_uattr_t){.uxn = BP_UXN, .pxn = BP_PXN, .contiguous = BP_NON_CONTIGUOUS};
    bp_lattr_t bpl = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    _pmap_enter(pmap_kernel(), temp_va, paddr, bpu, bpl);
}


void _pmap_update_pte(vaddr_t va, unsigned int asid, pte_t *old_pte, pte_t new_pte) {
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
    tlb_invalidate_va(va, asid);

    // 3. Write new pte and issue DSB
    *old_pte = new_pte;
    barrier_dsb();
}

void _pmap_clear_pte(vaddr_t va, unsigned int asid, pte_t *old_pte) {
    *old_pte = 0;
    if (IS_PTE_VALID(*old_pte)) {
        barrier_dsb();
        tlb_invalidate_va(va, asid);
    }
}

void _pmap_clear_pte_no_tlbi(pte_t *old_pte) {
    *old_pte = 0;
}

#define GET_TABLE_IDX(va, lsb, mask)  ((((va) & 0xFFFFFFFFFFFF) >> (lsb)) & (mask))

pte_t _pmap_insert_table(pmap_t *pmap, pte_t *parent_table, unsigned long index, unsigned long idx_width, unsigned long level, bool mmu_enabled, bool is_current_ttb) {
    pte_t pte = _pmap_alloc_table(pmap), *ptep = &parent_table[index];
    pte_t *new_table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)parent_table, index, idx_width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)new_table, PTE_TO_PA(pte));

    _pmap_update_pte((vaddr_t)new_table, pmap->asid, ptep, pte);
    memset((void*)new_table, 0, PAGESIZE);

    return pte;
}

void _pmap_remove_table(pmap_t *pmap, pte_t *table, pte_t *parent_table_pte) {
    paddr_t table_pa = PTE_TO_PA(*parent_table_pte);
    _pmap_clear_pte((vaddr_t)table, pmap->asid, parent_table_pte);
    vm_page_free(vm_page_from_pa(table_pa));
}

bool _pmap_is_table_empty(pte_t *table) {
    bool empty = true;
    for (int i = 0; i < MAX_NUM_PTES_LL; i++) {
        if (IS_PTE_VALID(table[i])) {
            empty = false;
            break;
        }
    }
    return empty;
}

void _pmap_enter(pmap_t *pmap, vaddr_t vaddr, paddr_t paddr, bp_uattr_t bpu, bp_lattr_t bpl) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0;
    unsigned long width = PAGESHIFT - 3;
    unsigned long mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width);
    unsigned long index = GET_TABLE_IDX(vaddr, lsb, mask);
    pte_t pte, *ptep;

    bool mmu_enabled = mmu_is_enabled();
    bool is_current_ttb = pmap->ttb == (mmu_get_ttbr0() & 0xFFFFFFFFFFFF) || pmap->ttb == (mmu_get_ttbr1() & 0xFFFFFFFFFFFF);

    pte_t *table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TTB_VA(pmap) : TEMP_PAGE_VA(0)) : pmap->ttb);
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Create a page table here if needed
        pte = table[index], ptep = &table[index];
        if (!IS_TDE_VALID(pte)) pte = _pmap_insert_table(pmap, table, index, width, level, mmu_enabled, is_current_ttb);

        // Get the address to the next table
        table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
        if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);
    }

    // Level 1
    pte = table[index], ptep = &table[index];
    if (!IS_TDE_VALID(pte)) pte = _pmap_insert_table(pmap, table, index, width, level, mmu_enabled, is_current_ttb);
    table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 2
    pte = table[index], ptep = &table[index];
    if (!IS_TDE_VALID(pte)) pte = _pmap_insert_table(pmap, table, index, width, level, mmu_enabled, is_current_ttb);
    table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 3 - Finally enter the mapping
    pte = table[index], ptep = &table[index];
    _pmap_update_pte(vaddr, pmap->asid, ptep, MAKE_PDE(paddr, bpu, bpl));
}

bool _pmap_remove(pmap_t *pmap, vaddr_t vaddr) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0;
    unsigned long width = PAGESHIFT - 3;
    unsigned long mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width);
    unsigned long index = GET_TABLE_IDX(vaddr, lsb, mask);
    pte_t pte, *ptep[4];

    bool mmu_enabled = mmu_is_enabled();
    bool is_current_ttb = pmap->ttb == (mmu_get_ttbr0() & 0xFFFFFFFFFFFF) || pmap->ttb == (mmu_get_ttbr1() & 0xFFFFFFFFFFFF);

    pte_t *table[4];
    table[level] = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TTB_VA(pmap) : TEMP_PAGE_VA(0)) : pmap->ttb);
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table[level], pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Check that a page table exists here
        pte = table[level][index], ptep[level] = &table[level][index];
        if (!IS_TDE_VALID(pte)) return false;

        // Get the address to the next table
        table[level+1] = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table[level], index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
        if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table[level+1], PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);
    }

    // Level 1
    pte = table[level][index], ptep[level] = &table[level][index];
    if (!IS_TDE_VALID(pte)) return false;
    table[level+1] = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table[level], index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table[level+1], PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 2
    pte = table[level][index], ptep[level] = &table[level][index];
    if (!IS_TDE_VALID(pte)) return false;
    table[level+1] = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table[level], index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table[level+1], PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 3 - Finally remove the mapping
    pte = table[level][index], ptep[level] = &table[level][index];
    if (!IS_PDE_VALID(pte)) return false;
    _pmap_clear_pte(vaddr, pmap->asid, ptep[level]);

    // Now scan the tables in the table walk hierarchy in reverse order, if the table is empty remove it from the parent table and the scan the parent table
    for (unsigned int l = 3; l > 0; l--) {
        if (_pmap_is_table_empty(table[l])) {
            _pmap_remove_table(pmap, table[l], ptep[l-1]);
        }
    }

    return true;
}

void _pmap_do_remove_all(pmap_t *pmap, pte_t *parent_table_pte, pte_t *table, unsigned long level) {
    // Calculate the table index width, mask and lsb for this levels index bits in the VA
    // The top 16 bits of the VA are not used in translation so clear them
    unsigned long width = PAGESHIFT - 3, mask = (1 << width) - 1, lsb = PAGESHIFT + ((3 - level) * width), max_num_ptes_ll = PAGESIZE >> 3;
    bool mmu_enabled = mmu_is_enabled();
    bool is_current_ttb = pmap->ttb == (mmu_get_ttbr0() & 0xFFFFFFFFFFFF) || pmap->ttb == (mmu_get_ttbr1() & 0xFFFFFFFFFFFF);

    // Skip the last entry in level 0; this is our recursive mapping
    for (unsigned long index = 0; index < (max_num_ptes_ll - (level == 0 ? 1 : 0)); index++) {
        pte_t pte = table[index], *ptep = &table[index];

        if (level == 3 && IS_PTE_VALID(pte)) {
            // Defer TLBI until all entries are cleared
            _pmap_clear_pte_no_tlbi(&table[index]);
        } else if (IS_TDE_VALID(pte)) {
            pte_t *next_table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
            if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)next_table, PTE_TO_PA(pte));
            _pmap_do_remove_all(pmap, ptep, next_table, level + 1);
        }
    }

    // Now remove it from the parent table and free the page if it's completely empty
    if (parent_table_pte != NULL) _pmap_remove_table(pmap, table, parent_table_pte);
}

void _pmap_remove_all(pmap_t *pmap) {
    bool mmu_enabled = mmu_is_enabled();
    bool is_current_ttb = pmap->ttb == (mmu_get_ttbr0() & 0xFFFFFFFFFFFF) || pmap->ttb == (mmu_get_ttbr1() & 0xFFFFFFFFFFFF);

    pte_t *table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TTB_VA(pmap) : TEMP_PAGE_VA(0)) : pmap->ttb);
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, pmap->ttb);

    _pmap_do_remove_all(pmap, NULL, table, 0);
    barrier_dsb();
    tlb_invalidate_asid(pmap->asid);
}

bool _pmap_lookup(pmap_t *pmap, vaddr_t vaddr, paddr_t *paddr, bp_uattr_t *bpu, bp_lattr_t *bpl) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0;
    unsigned long width = PAGESHIFT - 3;
    unsigned long mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width);
    unsigned long index = GET_TABLE_IDX(vaddr, lsb, mask);
    pte_t pte;

    bool mmu_enabled = mmu_is_enabled();
    bool is_current_ttb = pmap->ttb == (mmu_get_ttbr0() & 0xFFFFFFFFFFFF) || pmap->ttb == (mmu_get_ttbr1() & 0xFFFFFFFFFFFF);

    pte_t *table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TTB_VA(pmap) : TEMP_PAGE_VA(0)) : pmap->ttb);
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Check that a page table exists here
        pte = table[index];
        if (!IS_TDE_VALID(pte)) return false;

        // Get the address to the next table
        table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
        if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);
    }

    // Level 1
    pte = table[index];
    if (!IS_TDE_VALID(pte)) return false;
    table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 2
    pte = table[index];
    if (!IS_TDE_VALID(pte)) return false;
    table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 3 - Finally translate the virtual address
    pte = table[index];
    if (!IS_PDE_VALID(pte)) return false;

    *paddr = PTE_TO_PA(pte) | (vaddr & (PAGESIZE - 1));
    *bpu = BP_UATTR_EXTRACT(pte);
    *bpl = BP_LATTR_EXTRACT(pte);
    return true;
}

void _pmap_protect(pmap_t *pmap, vaddr_t vaddr, bp_uattr_t bpu, bp_lattr_t bpl) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0;
    unsigned long width = PAGESHIFT - 3;
    unsigned long mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width);
    unsigned long index = GET_TABLE_IDX(vaddr, lsb, mask);
    pte_t pte, *ptep;

    bool mmu_enabled = mmu_is_enabled();
    bool is_current_ttb = pmap->ttb == (mmu_get_ttbr0() & 0xFFFFFFFFFFFF) || pmap->ttb == (mmu_get_ttbr1() & 0xFFFFFFFFFFFF);

    pte_t *table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TTB_VA(pmap) : TEMP_PAGE_VA(0)) : pmap->ttb);
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Since we are updating the protections for this virtual address we expect there to be a valid table here
        pte = table[index], ptep = &table[index];
        kassert(IS_TDE_VALID(pte));

        // Get the address to the next table
        table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
        if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);
    }

    // Level 1
    pte = table[index], ptep = &table[index];
    kassert(IS_TDE_VALID(pte));
    table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 2
    pte = table[index], ptep = &table[index];
    kassert(IS_TDE_VALID(pte));
    table = (pte_t*)(mmu_enabled ? (is_current_ttb ? GET_TABLE_VA(pmap, (vaddr_t)table, index, width) : TEMP_PAGE_VA(level+1)) : PTE_TO_PA(pte));
    if (mmu_enabled && !is_current_ttb) _pmap_map_temp_page((vaddr_t)table, PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(vaddr, lsb, mask);

    // Level 3 - Finally update the mapping
    pte = table[index], ptep = &table[index];
    kassert(IS_PDE_VALID(pte));

    paddr_t pa = PTE_TO_PA(pte);

    // Only update the AP, UXN and PXN attributes
    bp_lattr_t new_bpl = BP_LATTR_EXTRACT(pte);
    bp_uattr_t new_bpu = BP_UATTR_EXTRACT(pte);
    new_bpl.ap = bpl.ap;
    new_bpu.uxn = bpu.uxn;
    new_bpu.pxn = bpu.pxn;

    _pmap_update_pte(vaddr, pmap->asid, ptep, MAKE_PDE(pa, new_bpu, new_bpl));
}

void pmap_bootstrap(void) {
    // This is one of the first routines that is called in kernel init. All it does is setup page tables and such
    // just enough in order to get the kernel running in virtual memory mode with the MMU on
    // Check for MMU supported features. We prefer 4KB pages
    PAGESIZE = mmu_is_4kb_granule_supported() ? _4KB : (mmu_is_16kb_granule_supported() ? _16KB : _64KB);
    PAGESHIFT = _ctz(PAGESIZE);

    kernel_pmap.lock = SPINLOCK_INIT;
    kernel_pmap.asid = 0;

    // Set the start and end of the kernel's virtual and physical address space
    // kernel_virtual_end is set to 0 at boot so that early bootstrap allocaters will kassert if they are called before pmap_init
    size_t kernel_size = ROUND_PAGE_UP(kernel_physical_end - kernel_physical_start);
    kernel_physical_end = kernel_physical_start + kernel_size;
    kernel_virtual_end = kernel_virtual_start + kernel_size;

    // The page allocation sub-system won't have any way to allocate memory this early in the boot process so pre-allocate memory for it
    size_t vm_page_array_size = ROUND_PAGE_UP((MEMSIZE >> PAGESHIFT) * sizeof(vm_page_t));
    vaddr_t vm_page_array_va = kernel_virtual_end;
    vm_page_init(kernel_physical_end);
    kernel_physical_end += vm_page_array_size;
    kernel_virtual_end += vm_page_array_size;
    kernel_size = kernel_virtual_end - kernel_virtual_start;

    // Reserve the physical pages used by the kernel
    for(unsigned long i = 0, num_pages = kernel_size >> PAGESHIFT; i < num_pages; i++) {
        vm_page_reserve_pa(kernel_physical_start + (i << PAGESHIFT));
    }

    // Let's grab a page for the base translation table
    // The base table for 16KB granule only has 2 entries while the 64KB granule base table only has 64 entries.
    kernel_pmap.ttb = PTE_TO_PA(_pmap_alloc_table(pmap_kernel()));
    _pmap_setup_table_recursive_mapping(&kernel_pmap);

    // Map the kernel's virtual address space to it's physical location
    for (size_t s = 0; s < kernel_size; s += PAGESIZE) {
        pmap_enter(pmap_kernel(), kernel_virtual_start + s, kernel_physical_start + s, VM_PROT_ALL, PMAP_FLAGS_WIRED | PMAP_FLAGS_WRITE_BACK);
    }

    // Now let's create temporary mappings to identity map the kernel's physical address space (needed when we enable the MMU)
    // We need to allocate a new TTB since these mappings will be in TTBR0 while the kernel virtual mappings are in TTBR1
    pmap_t identity_pmap;
    identity_pmap.lock = SPINLOCK_INIT;
    identity_pmap.ttb = PTE_TO_PA(_pmap_alloc_table(&identity_pmap));
    identity_pmap.asid = 0;
    _pmap_setup_table_recursive_mapping(&identity_pmap);

    // These mappings will be just thrown out after, use the internal _pmap_enter for this
    bp_uattr_t bp_uattr_page = (bp_uattr_t){.uxn = BP_UXN, .pxn = BP_NON_PXN, .contiguous = BP_NON_CONTIGUOUS};
    bp_lattr_t bp_lattr_page = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};
    for (size_t s = 0; s < kernel_size; s += PAGESIZE) {
        _pmap_enter(&identity_pmap, kernel_physical_start + s, kernel_physical_start + s, bp_uattr_page, bp_lattr_page);
    }

    // Finally enable the MMU!
    ma_index_t ma_index = {.attrs = {MA_DEVICE_NGNRNE, MA_DEVICE_NGNRE, MA_NORMAL_NC, MA_NORMAL_INC, MA_NORMAL_WBWARA, MA_NORMAL_WTWARA, MA_NORMAL_WTWNRA, MA_NORMAL_WTWNRN}};
    mmu_enable(identity_pmap.ttb, kernel_pmap.ttb, MAIR(ma_index), PAGESIZE);
    mmu_kernel_longjmp(kernel_physical_start, kernel_virtual_start);

    // Re-locate the vm_page_array to it's virtual address
    vm_page_relocate_array(vm_page_array_va);

    // Reclaim page table memory from the identity mappings that we no longer need
    mmu_clear_ttbr0();
    pmap_remove_all(&identity_pmap);
    vm_page_free(vm_page_from_pa(identity_pmap.ttb));

    // Finally increment the reference count on the pmap. The refcount for kernel_pmap should never be 0.
    pmap_reference(pmap_kernel());
}

void pmap_init(void) {
}

void pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp) {
    kassert(vstartp != NULL && vendp != NULL);
    *vstartp = kernel_virtual_start;
    *vendp = kernel_virtual_end;
}

vaddr_t pmap_steal_memory(size_t vsize, vaddr_t *vstartp, vaddr_t *vendp) {
    static vaddr_t next_unused_addr = 0;
    if (next_unused_addr == 0) next_unused_addr = kernel_virtual_end;

    if ((next_unused_addr + vsize) > kernel_virtual_end) {
        // Allocate some more pages if we don't have more room at the end of the kernel for vsize
        size_t num_pages = ROUND_PAGE_UP(vsize - (kernel_virtual_end - next_unused_addr));
        for (unsigned long i = 0; i < num_pages; i++) {
            vm_page_t *page = vm_page_alloc();
            kassert(page != NULL);

            page->status.wired_count++;
            pmap_kenter_pa(kernel_virtual_end, vm_page_to_pa(page), VM_PROT_DEFAULT, PMAP_FLAGS_WRITE_BACK | PMAP_FLAGS_WIRED);
            kernel_virtual_end += PAGESIZE;
        }
    }

    *vstartp = kernel_virtual_start;
    *vendp = kernel_virtual_end;

    vaddr_t va = next_unused_addr;
    next_unused_addr += vsize;
    return va;
}

pmap_t* pmap_create(void) {
    return NULL;
}

void pmap_destroy(pmap_t *pmap) {
    kassert(pmap != NULL);
    atomic_dec(&pmap->refcount);

    if (pmap->refcount == 0) {
        pmap_remove_all(pmap);
    }

    // FIXME free the pmap
}

void pmap_reference(pmap_t *pmap) {
    kassert(pmap != NULL);
    atomic_inc(&pmap->refcount);
}

int pmap_enter(pmap_t *pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, pmap_flags_t flags) {
    kassert(pmap != NULL);

    // Make sure access type in flags don't exceed the protections being applied to the page
    kassert((flags & VM_PROT_ALL) <= prot);

    bp_uattr_t bpu;
    bp_lattr_t bpl;

    // Different sets of attributes for kernel vs user mappings
    if (pmap == pmap_kernel()) {
        bpu = (bp_uattr_t){
            .uxn = BP_UXN,
            .pxn = (prot & VM_PROT_EXECUTE) ? BP_NON_PXN : BP_PXN,
            .contiguous = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_GLOBAL,
            .af = BP_AF,
            .sh = BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW_NO_EL0 : BP_AP_RO_NO_EL0,
            .ns = BP_NON_SECURE,
            .ma = (flags & PMAP_FLAGS_NOCACHE) ? BP_MA_DEVICE_NGNRNE : ((flags & PMAP_FLAGS_WRITE_COMBINE) ? BP_MA_NORMAL_NC : BP_MA_NORMAL_WBWARA)
        };
    } else {
        bpu = (bp_uattr_t){
            .uxn = (prot & VM_PROT_EXECUTE) ? BP_NON_UXN : BP_UXN,
            .pxn = BP_PXN,
            .contiguous = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_NON_GLOBAL,
            .af = BP_AF,
            .sh = BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW : BP_AP_RO,
            .ns = BP_NON_SECURE,
            .ma = (flags & PMAP_FLAGS_NOCACHE) ? BP_MA_DEVICE_NGNRNE : ((flags & PMAP_FLAGS_WRITE_COMBINE) ? BP_MA_NORMAL_NC : BP_MA_NORMAL_WBWARA)
        };
    }

    // Map in one page
    spinlock_writeacquire(&pmap->lock);
    _pmap_enter(pmap, va, pa, bpu, bpl);

    // Update the vm_page struct (assume is_active is already set by a vm_page_alloc call somewhere else);
    vm_page_t *page = vm_page_from_pa(pa);
    if (flags & PMAP_FLAGS_WIRED) {
        page->status.wired_count++;
        pmap->stats.wired_count++;
    }

    if ((flags & VM_PROT_READ) || (flags & VM_PROT_EXECUTE)) {
        page->status.is_referenced = 1;
    }

    if (flags & VM_PROT_WRITE) {
        page->status.is_dirty = 1;
    }

    pmap->stats.resident_count++;
    spinlock_writerelease(&pmap->lock);

    return 0;
}

void pmap_remove(pmap_t *pmap, vaddr_t sva, vaddr_t eva) {
    kassert(pmap != NULL && eva >= sva);

    // Calculate the size of the region to remove, rounded up to the next page
    size_t size = ROUND_PAGE_UP(eva - sva);

    spinlock_writeacquire(&pmap->lock);
    for (size_t s = 0; s < size; s += PAGESIZE) {
        _pmap_remove(pmap, sva + s);
    }
    spinlock_writerelease(&pmap->lock);
}

void pmap_remove_all(pmap_t *pmap) {
    kassert(pmap != NULL);

    spinlock_writeacquire(&pmap->lock);
    _pmap_remove_all(pmap);
    spinlock_writerelease(&pmap->lock);
}

void pmap_protect(pmap_t *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot) {
    kassert(pmap != NULL);

    bp_uattr_t bpu;
    bp_lattr_t bpl;
    size_t size = ROUND_PAGE_UP(eva - sva);

    // Different sets of attributes for kernel vs user mappings
    if (pmap == pmap_kernel()) {
        bpu = (bp_uattr_t){
            .uxn = BP_UXN,
            .pxn = (prot & VM_PROT_EXECUTE) ? BP_NON_PXN : BP_PXN,
            .contiguous = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_GLOBAL,
            .af = BP_AF,
            .sh = BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW_NO_EL0 : BP_AP_RO_NO_EL0,
            .ns = BP_NON_SECURE,
            .ma = BP_MA_NORMAL_WBWARA
        };
    } else {
        bpu = (bp_uattr_t){
            .uxn = (prot & VM_PROT_EXECUTE) ? BP_NON_UXN : BP_UXN,
            .pxn = BP_PXN,
            .contiguous = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_NON_GLOBAL,
            .af = BP_AF,
            .sh = BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW : BP_AP_RO,
            .ns = BP_NON_SECURE,
            .ma = BP_MA_NORMAL_WBWARA
        };
    }

    spinlock_writeacquire(&pmap->lock);
    for (size_t s = 0; s < size; s += PAGESIZE) {
        _pmap_protect(pmap, sva + s, bpu, bpl);
    }
    spinlock_writerelease(&pmap->lock);
}

void pmap_unwire(pmap_t *pmap, vaddr_t va) {
    kassert(pmap != NULL);

    paddr_t pa;
    bp_uattr_t bpu;
    bp_lattr_t bpl;

    kassert(_pmap_lookup(pmap, va, &pa, &bpu, &bpl));

    vm_page_t *page = vm_page_from_pa(pa);
    page->status.wired_count--;

    spinlock_writeacquire(&pmap->lock);
    pmap->stats.wired_count--;
    spinlock_writerelease(&pmap->lock);
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pa) {
    kassert(pa != NULL && pmap != NULL);

    bp_uattr_t bpu;
    bp_lattr_t bpl;

    if (_pmap_lookup(pmap, va, pa, &bpu, &bpl)) return true;
    else return false;
}

void pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, pmap_flags_t flags) {
    bp_uattr_t bpu = (bp_uattr_t){
        .uxn = BP_UXN,
        .pxn = (prot & VM_PROT_EXECUTE) ? BP_NON_PXN : BP_PXN,
        .contiguous = BP_NON_CONTIGUOUS
    };
    bp_lattr_t bpl = (bp_lattr_t){
        .ng = BP_GLOBAL,
        .af = BP_AF,
        .sh = BP_ISH,
        .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW_NO_EL0 : BP_AP_RO_NO_EL0,
        .ns = BP_NON_SECURE,
        .ma = (flags & PMAP_FLAGS_NOCACHE) ? BP_MA_DEVICE_NGNRNE : ((flags & PMAP_FLAGS_WRITE_COMBINE) ? BP_MA_NORMAL_NC : BP_MA_NORMAL_WBWARA)
    };

    spinlock_writeacquire(&kernel_pmap.lock);
    _pmap_enter(pmap_kernel(), va, pa, bpu, bpl);
    kernel_pmap.stats.wired_count++;
    kernel_pmap.stats.resident_count++;
    spinlock_writerelease(&kernel_pmap.lock);
}

void pmap_kremove(vaddr_t va, size_t size) {
    spinlock_writeacquire(&kernel_pmap.lock);
    for (size_t s = 0; s < size; s += PAGESIZE) {
        _pmap_remove(pmap_kernel(), va + s);
    }
    spinlock_writerelease(&kernel_pmap.lock);
}

void pmap_copy(pmap_t *dst_map, pmap_t *src_map, vaddr_t dst_addr, size_t len, vaddr_t src_addr) {
    kassert(src_map != NULL && dst_map != NULL);

    len = ROUND_PAGE_UP(len);
    src_addr = ROUND_PAGE_DOWN(src_addr);
    dst_addr = ROUND_PAGE_DOWN(dst_addr);

    size_t stride_size = 0;
    for (unsigned long s = 0; s < len; s += PAGESIZE) {
        paddr_t pa;
        bp_uattr_t bpu;
        bp_lattr_t bpl;

        kassert(_pmap_lookup(src_map, src_addr + s, &pa, &bpu, &bpl));
        _pmap_enter(dst_map, dst_addr + s, pa & (PAGESIZE - 1), bpu, bpl);
    }
}

void pmap_activate(pmap_t *pmap) {
    kassert(pmap != NULL && pmap != pmap_kernel());
    mmu_set_ttbr0(pmap->ttb, pmap->asid);
}

void pmap_deactivate(pmap_t *pmap) {
    kassert(pmap != NULL && pmap != pmap_kernel());

    // Check that we are deactivating the current context
    unsigned long ttbr0 = mmu_get_ttbr0();
    kassert((ttbr0 >> 48) == pmap->asid && (ttbr0 & ~0xFFFF000000000000) == pmap->ttb);

    mmu_clear_ttbr0();
}

void pmap_zero_page(paddr_t pa) {
    _pmap_map_temp_page(TEMP_PAGE_VA0, pa);
    _zero_pages(TEMP_PAGE_VA0, PAGESIZE);
    barrier_dmb();
}

void pmap_copy_page(paddr_t src, paddr_t dst) {
    _pmap_map_temp_page(TEMP_PAGE_VA0, src);
    _pmap_map_temp_page(TEMP_PAGE_VA1, dst);
    _copy_pages(TEMP_PAGE_VA1, TEMP_PAGE_VA0, PAGESIZE);
    barrier_dmb();
}

void pmap_page_protect(vm_page_t *vpg, vm_prot_t prot) {
}

bool pmap_clear_modify(vm_page_t *vpg) {
    bool dirty = vpg->status.is_dirty;
    vpg->status.is_dirty = 0;
    return dirty;
}

bool pmap_clear_reference(vm_page_t *vpg) {
    bool referenced = vpg->status.is_referenced;
    vpg->status.is_referenced = 0;
    return referenced;
}
