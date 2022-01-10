/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel/kassert.h>
#include <kernel/list.h>
#include <kernel/kmem_slab.h>
#include <kernel/vm/vm_object.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/arch_atomic.h>
#include <kernel/arch/arch_mmu.h>
#include <kernel/arch/arch_barrier.h>
#include <kernel/arch/pmap.h>

#define _4KB   (0x1000)
#define _16KB  (0x4000)
#define _64KB  (0x10000)
#define _1GB   (0x40000000)
#define _4GB   (0x100000000)

// Upper attributes for page table descriptors

// NSTable values
typedef enum {
    T_SECURE     = 0,                  // Tables with this attribute are in the secure PA space. Subsequent levels of
                                       // lookup keep their NS or NSTable meanings
    T_NON_SECURE = 0x8000000000000000, // Tables with this attribute are in the non-secure PA space. As a results
                                       // subsequent levels of lookups are also forced non-secure
} t_ns_attr_t;

// APTable values
typedef enum {
    T_AP_NONE      = 0,                  // No effect on permissions on lower levels of lookups
    T_AP_NO_EL0    = 0x2000000000000000, // Access at EL0 not permitted, regardless of permissions at lower levels of
                                         // lookup
    T_AP_RO        = 0x4000000000000000, // Write access not permitted at any exception level regardless of permissions
                                         // at lower levels of lookup
    T_AP_RO_NO_EL0 = 0x6000000000000000, // Write access not permitted at any exception level and read access not
                                         // permitted at EL0 regardless of permissions at lower levels of lookup
} t_ap_attr_t;

// UXNTable values
typedef enum {
    T_NON_UXN = 0,                  // No effect on subsequent levels of lookups
    T_UXN     = 0x1000000000000000, // Sets the unprivileged execute-never attribute at EL0 which applies to all
                                    // subsequent levels of lookups
} t_uxn_attr_t;

// PXNTable values
typedef enum {
    T_NON_PXN = 0,                  // No effect on subsequent levels of lookups
    T_PXN     = 0x0800000000000000, // Sets the execute-never attribute at EL1 or higher exception levels and applies
                                    // to all subsequent levels of lookups
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
    BP_CONTIGUOUS     = 0x0010000000000000, // Sets the contiguous which can be used to hint the MMU hardware that this
                                            // entry is part of a contiguous set of entries that the TLB can cache in a
                                            // single TLB entry
} bp_ctg_attr_t;

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
    BP_OSH = 0x200, // Outer shareable. Data in this memory region is required to be coherent to agents in the outer
                    // shareable domain
    BP_ISH = 0x300, // Inner shareable. Data in this memory region is required to be coherent to agents in the inner
                    // shareable domain
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
    BP_MA_DEVICE_NGNRNE = 0,    // Device nGnRnE, non-gathering, non-reordering, non-early write acknowledgment
    BP_MA_DEVICE_NGNRE  = 0x4,  // Device nGnRE, non-gathering, non-reordering with early write acknowledgement
    BP_MA_NORMAL_NC     = 0x8,  // Outer & inner non-cacheable. Outer cacheability typically implies the last level
                                // cache
    BP_MA_NORMAL_INC    = 0xc,  // Inner non-cacheable only
    BP_MA_NORMAL_WBWARA = 0x10, // Outer & inner write-back/write-allocate/read-allocate
    BP_MA_NORMAL_WTWARA = 0x14, // Outer & inner write-through/write-allocate/read-allocate
    BP_MA_NORMAL_WTWNRA = 0x18, // Outer & inner write-through/no write-allocate/read-allocate
    BP_MA_NORMAL_WTWNRN = 0x1c, // Outer & inner write-through/no write-allocate/no read-allocate
} bp_ma_attr_t;

typedef struct {
    bp_uxn_attr_t uxn;
    bp_pxn_attr_t pxn;
    bp_ctg_attr_t ctg;
} bp_uattr_t;

typedef struct {
    bp_ng_attr_t ng;
    bp_af_attr_t af;
    bp_sh_attr_t sh;
    bp_ap_attr_t ap;
    bp_ns_attr_t ns;
    bp_ma_attr_t ma;
} bp_lattr_t;

#define BP_UATTR(bp_uattr) ((bp_uattr).uxn | (bp_uattr).pxn | (bp_uattr).ctg)
#define BP_UATTR_EXTRACT(pte)\
((bp_uattr_t){\
    .uxn = (bp_uxn_attr_t)((pte) & BP_UXN),\
    .pxn = (bp_pxn_attr_t)((pte) & BP_PXN),\
    .ctg = (bp_ctg_attr_t)((pte) & BP_CONTIGUOUS),\
})
#define BP_LATTR(bp_lattr) ((bp_lattr).ng | (bp_lattr).af | (bp_lattr).af | (bp_lattr).sh | (bp_lattr).ap |\
    (bp_lattr).ns | (bp_lattr).ma)
#define BP_LATTR_EXTRACT(pte)\
((bp_lattr_t){\
    .ng = (bp_ng_attr_t)((pte) & BP_NON_GLOBAL),\
    .af = (bp_af_attr_t)((pte) & BP_AF),\
    .sh = (bp_sh_attr_t)((pte) & BP_ISH),\
    .ap = (bp_ap_attr_t)((pte) & BP_AP_RO),\
    .ns = (bp_ns_attr_t)((pte) & BP_NON_SECURE),\
    .ma = (bp_ma_attr_t)((pte) & BP_MA_NORMAL_WTWNRN)\
})

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
#define MAIR(mair) ((long)(mair).attrs[0] | ((long)(mair).attrs[1] << 8) | ((long)(mair).attrs[2] << 16) |\
    ((long)(mair).attrs[3] << 24) | ((long)(mair).attrs[4] << 32) | ((long)(mair).attrs[5] << 40) |\
    ((long)(mair).attrs[6] << 48) | ((long)(mair).attrs[7] << 56))

#define MAX_NUM_PTES_TTB                       (1 << (48 - (PAGESHIFT + ((3l - (PAGESHIFT == 16 ? 1 : 0)) *\
    (PAGESHIFT - 3l)))))
#define MAX_NUM_PTES_LL                        (PAGESIZE >> 3)

#define BLOCK_SIZE                             (1l << (PAGESHIFT + PAGESHIFT - 3l))
#define IS_BLOCK_ALIGNED(addr)                 (((long)(addr) & (BLOCK_SIZE - 1l)) == 0)

#define PTE_TO_PA(pte)                         (((pte) & 0xffffffffffff) & -(PAGESIZE))
#define PA_TO_PTE(pa)                          PTE_TO_PA(pa)
#define TABLE_PA_TO_KVA(table_pa)              PA_TO_KVA(table_pa)
#define TABLE_KVA_TO_PA(table_kva)             KVA_TO_PA(table_kva)
#define MAKE_TDE(pa)                           (PA_TO_PTE(pa) | 0x3)
#define MAKE_BDE(pa, bp_uattr, bp_lattr)       (PA_TO_PTE(pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x1)
#define MAKE_PDE(pa, bp_uattr, bp_lattr)       (PA_TO_PTE(pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x3)

#define IS_PTE_VALID(pte)                      ((pte) & 0x1)
#define IS_PDE_VALID(pte)                      (((pte) & 0x3) == 0x3)
#define IS_TDE_VALID(pte)                      IS_PDE_VALID(pte)
#define IS_BDE_VALID(pte)                      (((pte) & 0x3) == 0x1)

#define GET_TABLE_IDX(va, lsb, mask)           ((((va) & 0xFFFFFFFFFFFF) >> (lsb)) & (mask))

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

// The last kernel virtual address the kernel grow up to
vaddr_t max_kernel_virtual_end;

unsigned long PAGESIZE;
unsigned long PAGESHIFT;

#define ASID_ALLOC()   (arch_atomic_inc(&asid_num))
#define GET_ASID(pmap) ((pmap->asid) & 0xff)
unsigned long asid_num;

// Struct to keep track of all PTEs mapping a specific page
typedef struct {
    pmap_t *pmap;           // Pointer to pmap mapping this page
    vaddr_t va;             // Virtual address mapping this page
    list_node_t ll_node;    // Linked list linkage
} pte_page_t;

// Array of all ptep_page_t lists, one per page
typedef struct {
    lock_t *lock;
    list_t *list;
} pte_page_list_t;

pte_page_list_t pte_page_list;

#define GET_PTE_PAGE_LIST_IDX(pa) (((pa) - MEMBASEADDR) >> PAGESHIFT)

// pte_page_t slab
#define PTE_PAGE_SLAB_NUM         (8192)
kmem_slab_t pte_page_slab;

// Page table slab
#define PAGE_TABLE_SLAB_NUM       (1024)
kmem_slab_t page_table_slab;

// pmap_t slab
#define PMAP_SLAB_NUM             (256)
kmem_slab_t pmap_slab;

list_compare_result_t _pmap_pte_page_search(list_node_t *n1, list_node_t *n2) {
    pte_page_t *p1 = list_entry(n1, pte_page_t, ll_node), *p2 = list_entry(n2, pte_page_t, ll_node);
    return (p1->pmap < p2->pmap) ? LIST_COMPARE_LT : (p1->pmap > p2->pmap) ? LIST_COMPARE_GT : LIST_COMPARE_EQ;
}

void _pmap_pte_page_insert(pmap_t *pmap, paddr_t pa, vaddr_t va) {
    pte_page_t *pte_page = (pte_page_t*)kmem_slab_alloc(&pte_page_slab);
    kassert(pte_page != NULL);

    pte_page->pmap = pmap;
    pte_page->va = va;
    list_node_init(&pte_page->ll_node);

    // Release the pmap lock, otherwise there's a possibility of deadlock with the pmap_page_protect function
    lock_release_exclusive(&pmap->lock);

    lock_acquire(&pte_page_list.lock[GET_PTE_PAGE_LIST_IDX(pa)]);
    kassert(list_push(&pte_page_list.list[GET_PTE_PAGE_LIST_IDX(pa)], &pte_page->ll_node));
    lock_release(&pte_page_list.lock[GET_PTE_PAGE_LIST_IDX(pa)]);

    lock_acquire_exclusive(&pmap->lock);
}

void _pmap_pte_page_remove(pmap_t *pmap, paddr_t pa) {
    // Release the pmap lock, otherwise there's a possibility of deadlock with the pmap_page_protect function
    lock_release_exclusive(&pmap->lock);

    lock_acquire(&pte_page_list.lock[GET_PTE_PAGE_LIST_IDX(pa)]);

    pte_page_t tmp = { .pmap = pmap };
    list_node_t *node = list_search(&pte_page_list.list[GET_PTE_PAGE_LIST_IDX(pa)], _pmap_pte_page_search,
        &tmp.ll_node);
    pte_page_t *pte_page = list_entry(node, pte_page_t, ll_node);

    kassert(pte_page != NULL);
    kassert(list_remove(&pte_page_list.list[GET_PTE_PAGE_LIST_IDX(pa)], &pte_page->ll_node));

    lock_release(&pte_page_list.lock[GET_PTE_PAGE_LIST_IDX(pa)]);

    lock_acquire_exclusive(&pmap->lock);

    kmem_slab_free(&pte_page_slab, pte_page);
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
    arch_barrier_dsb();

    // 2. Invalidate the TLB entry by VA
    arch_tlb_invalidate_va(va, (unsigned long)asid);

    // 3. Write new pte and issue DSB
    *old_pte = new_pte;
    arch_barrier_dsb();
}

void _pmap_clear_pte(vaddr_t va, unsigned int asid, pte_t *old_pte) {
    *old_pte = 0;
    if (IS_PTE_VALID(*old_pte)) {
        arch_barrier_dsb();
        arch_tlb_invalidate_va(va, (unsigned long)asid);
    }
}

void _pmap_clear_pte_no_tlbi(pte_t *old_pte) {
    *old_pte = 0;
}

pte_t _pmap_insert_table(pmap_t *pmap, pte_t *parent_table_pte) {
    vaddr_t new_table_va = (vaddr_t)kmem_slab_zalloc(&page_table_slab);
    kassert(new_table_va != 0);

    *parent_table_pte = MAKE_TDE(TABLE_KVA_TO_PA(new_table_va));
    return *parent_table_pte;
}

void _pmap_remove_table(pmap_t *pmap, pte_t *parent_table_pte) {
    paddr_t table_pa = PTE_TO_PA(*parent_table_pte);
    *parent_table_pte = 0;
    kmem_slab_free(&page_table_slab, (void*)TABLE_PA_TO_KVA(table_pa));
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

pte_t* _pmap_enter(pmap_t *pmap, vaddr_t va, paddr_t pa, bp_uattr_t bpu, bp_lattr_t bpl) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0, width = PAGESHIFT - 3, mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width), index = GET_TABLE_IDX(va, lsb, mask);
    pte_t pte, *ptep;

    if (pmap->ttb == 0) {
        vaddr_t ttb_va = (vaddr_t)kmem_slab_zalloc(&page_table_slab);
        kassert(ttb_va != 0);

        pmap->ttb = TABLE_KVA_TO_PA(ttb_va);
    }

    pte_t *table = (pte_t*)TABLE_PA_TO_KVA(pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Create a page table here if needed
        pte = table[index], ptep = &table[index];
        if (!IS_TDE_VALID(pte)) pte = _pmap_insert_table(pmap, ptep);

        // Get the address to the next table
        table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);
    }

    // Level 1
    pte = table[index], ptep = &table[index];
    if (!IS_TDE_VALID(pte)) pte = _pmap_insert_table(pmap, ptep);
    table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 2
    pte = table[index], ptep = &table[index];
    if (!IS_TDE_VALID(pte)) pte = _pmap_insert_table(pmap, ptep);
    table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 3 - Finally enter the mapping
    pte = table[index], ptep = &table[index];
    _pmap_update_pte(va, pmap->asid, ptep, MAKE_PDE(pa, bpu, bpl));

    return ptep;
}

pte_t* _pmap_remove(pmap_t *pmap, vaddr_t va) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0, width = PAGESHIFT - 3, mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width), index = GET_TABLE_IDX(va, lsb, mask);
    pte_t pte, *ptep[4];
    pte_t *table[4];

    if (pmap->ttb == 0) return NULL;
    table[level] = (pte_t*)TABLE_PA_TO_KVA(pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Check that a page table exists here
        pte = table[level][index], ptep[level] = &table[level][index];
        if (!IS_TDE_VALID(pte)) return NULL;

        // Get the address to the next table
        table[level+1] = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);
    }

    // Level 1
    pte = table[level][index], ptep[level] = &table[level][index];
    if (!IS_TDE_VALID(pte)) return NULL;
    table[level+1] = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 2
    pte = table[level][index], ptep[level] = &table[level][index];
    if (!IS_TDE_VALID(pte)) return NULL;
    table[level+1] = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 3 - Finally remove the mapping
    pte = table[level][index], ptep[level] = &table[level][index];
    if (!IS_PDE_VALID(pte)) return NULL;
    _pmap_clear_pte(va, pmap->asid, ptep[level]);

    // Now scan the tables in the table walk hierarchy in reverse order, if the table is empty remove it from the
    // parent table and the scan the parent table
    for (unsigned int l = 3; l >= 0; l--) {
        if (_pmap_is_table_empty(table[l])) {
            // Just free the base translation table
            if (l == 0) {
                kmem_slab_free(&page_table_slab, (void*)TABLE_PA_TO_KVA(pmap->ttb));
                pmap->ttb = 0;
            } else {
                _pmap_remove_table(pmap, ptep[l-1]);
            }
        }
    }

    return ptep[level];
}

bool _pmap_lookup(pmap_t *pmap, vaddr_t va, paddr_t *pa, bp_uattr_t *bpu, bp_lattr_t *bpl) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0, width = PAGESHIFT - 3, mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width), index = GET_TABLE_IDX(va, lsb, mask);
    pte_t pte;

    if (pmap->ttb == 0) return false;
    pte_t *table = (pte_t*)TABLE_PA_TO_KVA(pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Check that a page table exists here
        pte = table[index];
        if (!IS_TDE_VALID(pte)) return false;

        // Get the address to the next table
        table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);
    }

    // Level 1
    pte = table[index];
    if (!IS_TDE_VALID(pte)) return false;
    table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 2
    pte = table[index];
    if (!IS_TDE_VALID(pte)) return false;
    table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 3 - Finally translate the virtual address
    pte = table[index];
    if (!IS_PDE_VALID(pte)) return false;

    *pa = PTE_TO_PA(pte) | (va & (PAGESIZE - 1));
    *bpu = BP_UATTR_EXTRACT(pte);
    *bpl = BP_LATTR_EXTRACT(pte);
    return true;
}

bool _pmap_protect(pmap_t *pmap, vaddr_t va, bp_uattr_t bpu, bp_lattr_t bpl) {
    unsigned long level = (PAGESIZE == _64KB) ? 1 : 0, width = PAGESHIFT - 3, mask = (1 << width) - 1;
    unsigned long lsb = PAGESHIFT + ((3 - level) * width), index = GET_TABLE_IDX(va, lsb, mask);
    pte_t pte, *ptep;

    if (pmap->ttb == 0) return false;
    pte_t *table = (pte_t*)TABLE_PA_TO_KVA(pmap->ttb);

    // Just get the next table if we are at level 0
    if (level == 0) {
        // Check that a page table exists here
        pte = table[index];
        if (!IS_TDE_VALID(pte)) return false;

        // Get the address to the next table
        table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
        level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);
    }

    // Level 1
    pte = table[index];
    if (!IS_TDE_VALID(pte)) return false;
    table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 2
    pte = table[index];
    if (!IS_TDE_VALID(pte)) return false;
    table = (pte_t*)TABLE_PA_TO_KVA(PTE_TO_PA(pte));
    level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

    // Level 3 - Finally update the mapping
    pte = table[index], ptep = &table[index];
    if (!IS_PDE_VALID(pte)) return false;

    paddr_t pa = PTE_TO_PA(pte);

    // Only update the AP, AF, UXN and PXN attributes
    bp_lattr_t new_bpl = BP_LATTR_EXTRACT(pte);
    bp_uattr_t new_bpu = BP_UATTR_EXTRACT(pte);
    new_bpl.ap = bpl.ap;
    new_bpl.af = bpl.af;
    new_bpu.uxn = bpu.uxn;
    new_bpu.pxn = bpu.pxn;

    _pmap_update_pte(va, pmap->asid, ptep, MAKE_PDE(pa, new_bpu, new_bpl));

    return true;
}

void pmap_bootstrap(void) {
    // This is one of the first routines that is called in kernel init. All it does is setup page tables and such
    // just enough in order to get the kernel running in virtual memory mode with the MMU on
    // Check for MMU supported features. We prefer 4KB pages
    PAGESIZE = arch_mmu_is_4kb_granule_supported() ? _4KB : (arch_mmu_is_16kb_granule_supported() ? _16KB : _64KB);
    PAGESHIFT = arch_ctz(PAGESIZE);

    lock_init(&kernel_pmap.lock);
    kernel_pmap.asid = 0;

    // Set the start and end of the kernel's virtual and physical address space
    // kernel_virtual_end is set to 0 at boot so that early bootstrap allocaters will kassert if they are called before
    // pmap_init
    size_t kernel_size = ROUND_PAGE_UP(kernel_physical_end - kernel_physical_start);
    kernel_physical_end = kernel_physical_start + kernel_size;
    kernel_virtual_end = kernel_virtual_start + kernel_size;
    max_kernel_virtual_end = 0xFFFFFFFF00000000;

    // Pre-allocate enough page tables to linearly map all of memory
    size_t num_l3_tables = (MEMSIZE >> PAGESHIFT) / MAX_NUM_PTES_LL;
    size_t num_l2_tables = (num_l3_tables > MAX_NUM_PTES_LL) ? num_l3_tables / MAX_NUM_PTES_LL : 1;
    size_t num_l1_tables = (PAGESIZE == _64KB) ? 1 :
        (num_l2_tables > MAX_NUM_PTES_LL) ? num_l2_tables / MAX_NUM_PTES_LL : 1;
    size_t num_l0_tables = (PAGESIZE == _64KB) ? 0 : 1;
    size_t total_tables_size = (num_l0_tables + num_l1_tables + num_l2_tables + num_l3_tables) * PAGESIZE;

    paddr_t tables = kernel_physical_end;
    kernel_physical_end += total_tables_size;
    kernel_virtual_end += total_tables_size;
    kernel_size += total_tables_size;

    arch_fast_zero((void*)tables, total_tables_size);
    kernel_pmap.ttb = tables;
    tables += PAGESIZE;

    bp_uattr_t bp_uattr_page = (bp_uattr_t){.uxn = BP_UXN, .pxn = BP_NON_PXN, .ctg = BP_NON_CONTIGUOUS};
    bp_lattr_t bp_lattr_page = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0,
        .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    for (size_t offset = 0; offset < MEMSIZE;) {
        vaddr_t va = kernel_virtual_start + offset;
        paddr_t pa = kernel_physical_start + offset;

        unsigned long level = (PAGESIZE == _64KB) ? 1 : 0, width = PAGESHIFT - 3, mask = (1 << width) - 1;
        unsigned long lsb = PAGESHIFT + ((3 - level) * width), index = GET_TABLE_IDX(va, lsb, mask);

        pte_t pte;
        pte_t *table = (pte_t*)pmap_kernel()->ttb;

        // Just get the next table if we are at level 0
        if (level == 0) {
            // Create a page table here if needed
            pte = table[index];
            if (!IS_TDE_VALID(pte)) {
                pte = MAKE_TDE(tables);
                table[index] = pte;
                tables += PAGESIZE;
            }

            // Get the address to the next table
            table = (pte_t*)PTE_TO_PA(pte);
            level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);
        }

        // Level 1
        pte = table[index];
        if (!IS_TDE_VALID(pte)) {
            pte = MAKE_TDE(tables);
            table[index] = pte;
            tables += PAGESIZE;
        }
        table = (pte_t*)PTE_TO_PA(pte);
        level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

        // Level 2
        pte = table[index];
        if (!IS_TDE_VALID(pte)) {
            pte = MAKE_TDE(tables);
            table[index] = pte;
            tables += PAGESIZE;
        }
        table = (pte_t*)PTE_TO_PA(pte);
        level++, lsb -= width, index = GET_TABLE_IDX(va, lsb, mask);

        // Level 3 - Finally enter the mapping
        pte = table[index];
        table[index] = MAKE_PDE(pa, bp_uattr_page, bp_lattr_page);
        offset += PAGESIZE;
    }

    // Now let's create temporary mappings to identity map the kernel's physical address space (needed when we enable
    // the MMU). We need to allocate a new TTB since these mappings will be in TTBR0 while the kernel virtual mappings
    // are in TTBR1
    pmap_t identity_pmap;
    paddr_t identity_tables = tables;
    arch_fast_zero((void*)identity_tables, total_tables_size);

    lock_init(&identity_pmap.lock);
    identity_pmap.ttb = identity_tables;
    identity_tables += PAGESIZE;
    identity_pmap.asid = 0;

    // These mappings will be just thrown out after
    for (size_t offset = 0; offset < kernel_size; offset += PAGESIZE) {
        paddr_t pa = kernel_physical_start + offset;

        unsigned long level = (PAGESIZE == _64KB) ? 1 : 0, width = PAGESHIFT - 3, mask = (1 << width) - 1;
        unsigned long lsb = PAGESHIFT + ((3 - level) * width), index = GET_TABLE_IDX(pa, lsb, mask);

        pte_t pte;
        pte_t *table = (pte_t*)identity_pmap.ttb;

        // Just get the next table if we are at level 0
        if (level == 0) {
            // Create a page table here if needed
            pte = table[index];
            if (!IS_TDE_VALID(pte)) {
                pte = MAKE_TDE(identity_tables);
                table[index] = pte;
                identity_tables += PAGESIZE;
            }

            // Get the address to the next table
            table = (pte_t*)PTE_TO_PA(pte);
            level++, lsb -= width, index = GET_TABLE_IDX(pa, lsb, mask);
        }

        // Level 1
        pte = table[index];
        if (!IS_TDE_VALID(pte)) {
            pte = MAKE_TDE(identity_tables);
            table[index] = pte;
            identity_tables += PAGESIZE;
        }
        table = (pte_t*)PTE_TO_PA(pte);
        level++, lsb -= width, index = GET_TABLE_IDX(pa, lsb, mask);

        // Level 2
        pte = table[index];
        if (!IS_TDE_VALID(pte)) {
            pte = MAKE_TDE(identity_tables);
            table[index] = pte;
            identity_tables += PAGESIZE;
        }
        table = (pte_t*)PTE_TO_PA(pte);
        level++, lsb -= width, index = GET_TABLE_IDX(pa, lsb, mask);

        // Level 3 - Finally enter the mapping
        pte = table[index];
        table[index] = MAKE_PDE(pa, bp_uattr_page, bp_lattr_page);
    }

    // Finally enable the MMU!
    ma_index_t ma_index = {.attrs = {MA_DEVICE_NGNRNE, MA_DEVICE_NGNRE, MA_NORMAL_NC, MA_NORMAL_INC, MA_NORMAL_WBWARA,
        MA_NORMAL_WTWARA, MA_NORMAL_WTWNRA, MA_NORMAL_WTWNRN}};
    arch_mmu_enable(identity_pmap.ttb, kernel_pmap.ttb, MAIR(ma_index), PAGESIZE);
    arch_mmu_kernel_longjmp(kernel_physical_start, kernel_virtual_start);

    arch_mmu_clear_ttbr0();

    // We just linearly mapped all of memory so adjust kernel_virtual_end to recognize this
    kernel_virtual_end = kernel_virtual_start + MEMSIZE;

    // Finally increment the reference count on the pmap. The refcnt for kernel_pmap should never be 0.
    pmap_reference(pmap_kernel());
}

void pmap_init(void) {
    // Setup the page table slab
    vaddr_t page_table_slab_va = (vaddr_t)pmap_steal_memory(PAGE_TABLE_SLAB_NUM * PAGESIZE, NULL, NULL);;
    kmem_slab_create_no_vm(&page_table_slab, PAGESIZE, PAGE_TABLE_SLAB_NUM, (void*)page_table_slab_va);

    // Allocate memory for the pte_page array
    size_t pte_page_array_size = (MEMSIZE >> PAGESHIFT) * sizeof(list_t);
    size_t pte_page_lock_size = (MEMSIZE >> PAGESHIFT) * sizeof(lock_t);
    pte_page_list.list = (list_t*)pmap_steal_memory(pte_page_array_size, NULL, NULL);
    pte_page_list.lock = (lock_t*)pmap_steal_memory(pte_page_lock_size, NULL, NULL);
    arch_fast_zero(pte_page_list.list, pte_page_array_size);
    arch_fast_zero(pte_page_list.lock, pte_page_lock_size);

    // Setup the pte_page_t slab
    vaddr_t pte_page_slab_va = pmap_steal_memory(PTE_PAGE_SLAB_NUM * sizeof(pte_page_t), NULL, NULL);
    kmem_slab_create_no_vm(&pte_page_slab, sizeof(pte_page_t), PTE_PAGE_SLAB_NUM, (void*)pte_page_slab_va);

    // Setup the pmap_t slab
    vaddr_t pmap_slab_va = pmap_steal_memory(PMAP_SLAB_NUM * sizeof(pmap_t), NULL, NULL);
    kmem_slab_create_no_vm(&pmap_slab, sizeof(pmap_t), PMAP_SLAB_NUM, (void*)pmap_slab_va);
}

void pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp) {
    kassert(vstartp != NULL && vendp != NULL);
    *vstartp = kernel_virtual_start;
    *vendp = kernel_virtual_end;
}

vaddr_t pmap_steal_memory(size_t vsize, vaddr_t *vstartp, vaddr_t *vendp) {
    static paddr_t next_unused_addr = 0;
    if (next_unused_addr == 0) next_unused_addr = kernel_physical_end;

    if ((next_unused_addr + vsize) > kernel_physical_end) {
        // Allocate some more pages if we don't have more room at the end of the kernel for vsize
        size_t num_pages = ROUND_PAGE_UP(vsize - (kernel_physical_end - next_unused_addr)) >> PAGESHIFT;
        for (unsigned long i = 0; i < num_pages; i++) {
            kernel_physical_end += PAGESIZE;
        }

        kassert((kernel_physical_end - MEMBASEADDR) <= MEMSIZE);
    }

    if (vstartp != NULL) *vstartp = kernel_virtual_start;
    if (vendp != NULL) *vendp = kernel_virtual_end;

    vaddr_t va = PA_TO_KVA(next_unused_addr);
    next_unused_addr += vsize;

    return va;
}

pmap_t* pmap_create(void) {
    pmap_t *pmap = kmem_slab_alloc(&pmap_slab);
    kassert(pmap != NULL);

    lock_init(&pmap->lock);
    pmap->ttb = 0;
    pmap->asid = ASID_ALLOC();
    pmap->refcnt = 0;
    pmap_reference(pmap);
    pmap->stats = (pmap_statistics_t){0};

    return pmap;
}

void pmap_destroy(pmap_t *pmap) {
    kassert(pmap != NULL && pmap != pmap_kernel());

    lock_acquire_exclusive(&pmap->lock);
    pmap->refcnt--;

    if (pmap->refcnt == 0) {
        // Assuming all mappings have been removed prior to calling this function i.e. all tables have been freed
        kmem_slab_free(&pmap_slab ,pmap);
        return;
    }

    lock_release_exclusive(&pmap->lock);
}

void pmap_reference(pmap_t *pmap) {
    kassert(pmap != NULL);
    lock_acquire_exclusive(&pmap->lock);
    pmap->refcnt++;
    lock_release_exclusive(&pmap->lock);
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
            .ctg = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_GLOBAL,
            .af = (prot & VM_PROT_ALL) ? BP_AF : BP_NO_AF,
            .sh = (flags & PMAP_FLAGS_NOCACHE) ? BP_OSH : BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW_NO_EL0 : BP_AP_RO_NO_EL0,
            .ns = BP_NON_SECURE,
            .ma = (flags & PMAP_FLAGS_NOCACHE) ? BP_MA_DEVICE_NGNRNE :
                ((flags & PMAP_FLAGS_WRITE_COMBINE) ? BP_MA_NORMAL_NC : BP_MA_NORMAL_WBWARA)
        };
    } else {
        bpu = (bp_uattr_t){
            .uxn = (prot & VM_PROT_EXECUTE) ? BP_NON_UXN : BP_UXN,
            .pxn = BP_PXN,
            .ctg = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_NON_GLOBAL,
            .af = (prot & VM_PROT_ALL) ? BP_AF : BP_NO_AF,
            .sh = (flags & PMAP_FLAGS_NOCACHE) ? BP_OSH : BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW : BP_AP_RO,
            .ns = BP_NON_SECURE,
            .ma = (flags & PMAP_FLAGS_NOCACHE) ? BP_MA_DEVICE_NGNRNE :
                ((flags & PMAP_FLAGS_WRITE_COMBINE) ? BP_MA_NORMAL_NC : BP_MA_NORMAL_WBWARA)
        };
    }

    // Map in one page
    lock_acquire_exclusive(&pmap->lock);
    pte_t *ptep = _pmap_enter(pmap, va, pa, bpu, bpl);
    _pmap_pte_page_insert(pmap, pa, va);

    if (flags & PMAP_FLAGS_WIRED) pmap->stats.wired_count++;

    pmap->stats.resident_count++;
    lock_release_exclusive(&pmap->lock);

    return 0;
}

void pmap_remove(pmap_t *pmap, vaddr_t sva, vaddr_t eva) {
    kassert(pmap != NULL && eva >= sva);

    // Calculate the size of the region to remove, rounded up to the next page
    sva = ROUND_PAGE_DOWN(sva);
    eva = ROUND_PAGE_UP(eva);

    paddr_t pa = 0;
    bp_uattr_t bpu = {0};
    bp_lattr_t bpl = {0};

    lock_acquire_exclusive(&pmap->lock);
    for (vaddr_t va = sva; va < eva; va += PAGESIZE) {
        if (_pmap_lookup(pmap, va, &pa, &bpu, &bpl)) {
            pte_t *ptep = _pmap_remove(pmap, va);
            _pmap_pte_page_remove(pmap, pa);
        }
    }
    lock_release_exclusive(&pmap->lock);
}

void pmap_protect(pmap_t *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot) {
    kassert(pmap != NULL && eva >= sva);

    bp_uattr_t bpu;
    bp_lattr_t bpl;

    // Different sets of attributes for kernel vs user mappings
    if (pmap == pmap_kernel()) {
        bpu = (bp_uattr_t){
            .uxn = BP_UXN,
            .pxn = (prot & VM_PROT_EXECUTE) ? BP_NON_PXN : BP_PXN,
            .ctg = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_GLOBAL,
            .af = (prot & VM_PROT_ALL) ? BP_AF : BP_NO_AF,
            .sh = BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW_NO_EL0 : BP_AP_RO_NO_EL0,
            .ns = BP_NON_SECURE,
            .ma = BP_MA_NORMAL_WBWARA
        };
    } else {
        bpu = (bp_uattr_t){
            .uxn = (prot & VM_PROT_EXECUTE) ? BP_NON_UXN : BP_UXN,
            .pxn = BP_PXN,
            .ctg = BP_NON_CONTIGUOUS
        };
        bpl = (bp_lattr_t){
            .ng = BP_NON_GLOBAL,
            .af = (prot & VM_PROT_ALL) ? BP_AF : BP_NO_AF,
            .sh = BP_ISH,
            .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW : BP_AP_RO,
            .ns = BP_NON_SECURE,
            .ma = BP_MA_NORMAL_WBWARA
        };
    }

    sva = ROUND_PAGE_DOWN(sva);
    eva = ROUND_PAGE_UP(eva);

    lock_acquire_exclusive(&pmap->lock);
    for (vaddr_t va = sva; va < eva; va += PAGESIZE) {
        _pmap_protect(pmap, va, bpu, bpl);
    }
    lock_release_exclusive(&pmap->lock);
}

void pmap_unwire(pmap_t *pmap, vaddr_t va) {
    kassert(pmap != NULL);

    paddr_t pa;
    bp_uattr_t bpu;
    bp_lattr_t bpl;

    kassert(_pmap_lookup(pmap, va, &pa, &bpu, &bpl));

    vm_page_t *page = vm_page_from_pa(pa);
    vm_page_unwire(page);

    lock_acquire_exclusive(&pmap->lock);
    pmap->stats.wired_count--;
    lock_release_exclusive(&pmap->lock);
}

bool pmap_extract(pmap_t *pmap, vaddr_t va, paddr_t *pa) {
    kassert(pa != NULL && pmap != NULL);

    bp_uattr_t bpu;
    bp_lattr_t bpl;

    lock_acquire_shared(&pmap->lock);
    bool ret = _pmap_lookup(pmap, va, pa, &bpu, &bpl);
    lock_release_shared(&pmap->lock);

    return ret;
}

void pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot, pmap_flags_t flags) {
    bp_uattr_t bpu = (bp_uattr_t){
        .uxn = BP_UXN,
        .pxn = (prot & VM_PROT_EXECUTE) ? BP_NON_PXN : BP_PXN,
        .ctg = BP_NON_CONTIGUOUS
    };
    bp_lattr_t bpl = (bp_lattr_t){
        .ng = BP_GLOBAL,
        .af = (prot & VM_PROT_ALL) ? BP_AF : BP_NO_AF,
        .sh = (flags & PMAP_FLAGS_NOCACHE) ? BP_OSH : BP_ISH,
        .ap = (prot & VM_PROT_WRITE) ? BP_AP_RW_NO_EL0 : BP_AP_RO_NO_EL0,
        .ns = BP_NON_SECURE,
        .ma = (flags & PMAP_FLAGS_NOCACHE) ? BP_MA_DEVICE_NGNRNE :
            ((flags & PMAP_FLAGS_WRITE_COMBINE) ? BP_MA_NORMAL_NC : BP_MA_NORMAL_WBWARA)
    };

    lock_acquire_exclusive(&kernel_pmap.lock);
    _pmap_enter(pmap_kernel(), va, pa, bpu, bpl);
    kernel_pmap.stats.wired_count++;
    kernel_pmap.stats.resident_count++;
    lock_release_exclusive(&kernel_pmap.lock);
}

void pmap_kremove(vaddr_t va, size_t size) {
    lock_acquire_exclusive(&kernel_pmap.lock);
    for (size_t s = 0; s < size; s += PAGESIZE) {
        _pmap_remove(pmap_kernel(), va + s);
    }
    lock_release_exclusive(&kernel_pmap.lock);
}

void pmap_copy(pmap_t *dst_map, pmap_t *src_map, vaddr_t dst_addr, size_t len, vaddr_t src_addr) {
    kassert(src_map != NULL && dst_map != NULL);

    len = ROUND_PAGE_UP(len);
    src_addr = ROUND_PAGE_DOWN(src_addr);
    dst_addr = ROUND_PAGE_DOWN(dst_addr);

    if (src_map < dst_map) {
        lock_acquire_exclusive(&dst_map->lock);
        lock_acquire_shared(&src_map->lock);
    } else {
        lock_acquire_shared(&src_map->lock);
        lock_acquire_exclusive(&dst_map->lock);
    }

    size_t stride_size = 0;
    for (unsigned long s = 0; s < len; s += PAGESIZE) {
        paddr_t pa;
        bp_uattr_t bpu;
        bp_lattr_t bpl;

        kassert(_pmap_lookup(src_map, src_addr + s, &pa, &bpu, &bpl));
        _pmap_enter(dst_map, dst_addr + s, pa & (PAGESIZE - 1), bpu, bpl);
    }

    if (src_map < dst_map) {
        lock_release_shared(&src_map->lock);
        lock_release_exclusive(&dst_map->lock);
    } else {
        lock_release_exclusive(&dst_map->lock);
        lock_release_shared(&src_map->lock);
    }
}

void pmap_activate(pmap_t *pmap) {
    kassert(pmap != NULL && pmap != pmap_kernel());
    lock_acquire_shared(&pmap->lock);
    arch_mmu_set_ttbr0(pmap->ttb, GET_ASID(pmap));
    lock_release_shared(&pmap->lock);
}

void pmap_deactivate(pmap_t *pmap) {
    kassert(pmap != NULL && pmap != pmap_kernel());

    lock_acquire_shared(&pmap->lock);

    // Check that we are deactivating the current context
    unsigned long ttbr0 = arch_mmu_get_ttbr0();
    kassert((ttbr0 >> 48) == pmap->asid && (ttbr0 & ~0xFFFF000000000000) == pmap->ttb);

    arch_mmu_clear_ttbr0();

    lock_release_shared(&pmap->lock);
}

void pmap_zero_page(paddr_t pa) {
    lock_acquire_exclusive(&pmap_kernel()->lock);
    arch_fast_zero((void*)PA_TO_KVA(pa), PAGESIZE);
    lock_release_exclusive(&pmap_kernel()->lock);
    arch_barrier_dmb();
}

void pmap_copy_page(paddr_t src, paddr_t dst) {
    lock_acquire_exclusive(&pmap_kernel()->lock);
    arch_fast_move((void*)PA_TO_KVA(dst), (void*)PA_TO_KVA(src), PAGESIZE);
    lock_release_exclusive(&pmap_kernel()->lock);
    arch_barrier_dmb();
}

void pmap_page_protect(paddr_t pa, vm_prot_t prot) {
    if (prot == VM_PROT_ALL) return;

    lock_acquire(&pte_page_list.lock[GET_PTE_PAGE_LIST_IDX(pa)]);

    pte_page_t *entry = NULL;
    list_for_each_entry(&pte_page_list.list[GET_PTE_PAGE_LIST_IDX(pa)], entry, ll_node) {
        vaddr_t eva = entry->va + PAGESIZE;
        pmap_protect(entry->pmap, entry->va, eva, prot);
    }

    lock_release(&pte_page_list.lock[GET_PTE_PAGE_LIST_IDX(pa)]);
}

bool pmap_clear_modify(vm_page_t *page) {
    lock_acquire_exclusive(&page->object->lock);
    bool dirty = page->status.is_dirty;
    page->status.is_dirty = 0;
    lock_release_exclusive(&page->object->lock);
    return dirty;
}

bool pmap_clear_reference(vm_page_t *page) {
    lock_acquire_exclusive(&page->object->lock);
    bool referenced = page->status.is_referenced;
    page->status.is_referenced = 0;
    lock_release_exclusive(&page->object->lock);
    return referenced;
}
