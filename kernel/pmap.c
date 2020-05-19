#include <kernel/pmap.h>
#include <kernel/pmm.h>
#include <kernel/mmu.h>
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

#define MAX_NUM_PTES_TTB(pmap) (1 << GET_TABLE_IDX_WIDTH_TTB((pmap).page_shift))
#define MAX_NUM_PTES_LL(pmap)  ((pmap).page_size >> 3)
#define BLOCK_SIZE(pmap)       (1 << ((pmap).page_shift + (pmap).page_shift - 3))

#define ROUND_PAGE_DOWN(pmap, addr)  ((long)(addr) & ~((long)(pmap).page_size - 1))
#define ROUND_PAGE_UP(pmap, addr)    ((IS_PAGE_ALIGNED((pmap), (addr))) ? (long)(addr) : (ROUND_PAGE_DOWN((pmap), (addr)) + (long)(pmap).page_size))

#define IS_PAGE_ALIGNED(pmap, addr)  (((long)(addr) & ((long)(pmap).page_size - 1)) == 0)
#define IS_BLOCK_ALIGNED(pmap, addr) (((long)(addr) & (BLOCK_SIZE(pmap) - 1)) == 0)

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

#define PTE_TO_PA(pmap, pte)  (((pte) & 0xffffffffffff) & -((pmap).page_size))
#define PA_TO_PTE(pmap, pa) PTE_TO_PA(pmap, pa)
#define MAKE_TDE(pmap, pa, t_attr, bp_lattr)   (PA_TO_PTE(pmap, pa) | T_ATTR(t_attr) | BP_LATTR(bp_lattr) | 0x3)
#define MAKE_BDE(pmap, pa, bp_uattr, bp_lattr) (PA_TO_PTE(pmap, pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x1)
#define MAKE_PDE(pmap, pa, bp_uattr, bp_lattr) (PA_TO_PTE(pmap, pa) | BP_UATTR(bp_uattr) | BP_LATTR(bp_lattr) | 0x3)

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


// Get the base virtual address of where the page tables are mapped
// This depends on the page size: 4KB - last 512GB of the virtual address space, 16KB - last 64GB, 64KB - last 512MB
// For non-kernel page tables, clear the high-order 16 bits
#define PT_VA_BASE(pmap) (((pmap).page_size == _4KB ? -(512l << 30) : ((pmap).page_size == _16KB ? -(64l << 30) : -(512l << 20))) & ~((pmap).is_kernel ? 0 : 0xffff000000000000))

// This macro takes a page table index and converts it into a VA that addresses that page table
// This works assuming the last virtual address region corresponding to a level 0/1/2 aperture for 4KB/16KB/64KB page granules
// is mapped to point the first page table in that level.
// So for a 4KB page granule. The last entry in the level 0 table (TTB) is mapped as a table entry that points back to the level 0 table.
// The page tables in this virtual address region can be accessed linearly by page starting with the level 3 page tables and then level 2
// and then level 1 and finally level 0.
// 16KB and 64KB page granules are kind of messy since the level 0, or level 1 for 64KB, tables do not hold the same number of entries
// as the lower level tables. To get around this, the last entry in the last table one level down is mapped to itself. We lose access
// to the TTB VA with this method however which is why the pmap struct holds the TTB va.
#define PT_IDX_TO_VA(pmap, pt_idx) (PT_VA_BASE(pmap) + (pt_idx) << (pmap).page_shift)

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

#include <kernel/kstdio.h>

paddr_t _pmap_init_map_range(vaddr_t vaddr, paddr_t paddr, size_t size, paddr_t next_unused_pa) {
    // Attributes for the kernel's table descriptors
    t_attr_t t_attr_table = (t_attr_t){.ns = T_NON_SECURE, .ap = T_AP_NO_EL0, .uxn = T_UXN, .pxn = T_NON_PXN};
    bp_lattr_t bp_lattr_table = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    // Attributes for the Kernel's page table mappings
    bp_uattr_t bp_uattr_page = (bp_uattr_t){.uxn = BP_UXN, .pxn = BP_NON_PXN, .contiguous = BP_NON_CONTIGUOUS};
    bp_lattr_t bp_lattr_page = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    // Now we need to map the kernel's virtual address space
    for (unsigned long page_offset = 0, block_size = BLOCK_SIZE(kernel_pmap); page_offset < size;) {
        pte_t *table = (pte_t*)kernel_pmap.ttb;

        // Get rid of extraneous VA bits not used in translation
        vaddr_t va = (vaddr + page_offset) & ~(0xFFFF000000000000);
        paddr_t pa = paddr + page_offset;

        // Walk the page tables checking if a table is missing at an intermediate level and if so allocate one using next_unused_pa
        unsigned int level = kernel_pmap.page_size == _64KB ? 1 : 0;
        unsigned int width = GET_TABLE_IDX_WIDTH(kernel_pmap.page_shift), mask = GET_TABLE_IDX_MASK(width), lsb = GET_TABLE_IDX_LSB(level, width, kernel_pmap.page_shift);
        for (; level < 3; level++, lsb -= width) {
            unsigned int index = GET_TABLE_IDX(va, lsb, mask);
            pte_t pte = table[index];

            // Check if we can make a block entry at level 2. We need to make sure the PA is block aligned and we have enough to map
            if (level == 2 && IS_BLOCK_ALIGNED(kernel_pmap, pa) && (size - page_offset) > block_size) break;

            if (!IS_TDE_VALID(pte)) {
                // Allocate a page for this new table and zero it out
                pte_t *next_table = (pte_t*)next_unused_pa;
                next_unused_pa += kernel_pmap.page_size;
                memset((void*)next_table, 0, kernel_pmap.page_size);

                // Add the TDE to the current table
                pte = MAKE_TDE(kernel_pmap, (paddr_t)next_table, t_attr_table, bp_lattr_table);
                table[index] = pte;
            }

            // Update table pointer to the next table
            table = (pte_t*)PTE_TO_PA(kernel_pmap, pte);
        }

        pte_t pte = level == 2 ? MAKE_BDE(kernel_pmap, pa, bp_uattr_page, bp_lattr_page) : MAKE_PDE(kernel_pmap, pa, bp_uattr_page, bp_lattr_page);
        table[GET_TABLE_IDX(va, lsb, mask)] = pte;
        page_offset += level == 2 ? block_size : kernel_pmap.page_size;
    }

    // Return the updated next_unused_pa;
    return next_unused_pa;
}

void pmap_init(void) {
    // Check for MMU supported features. We prefer 4KB pages
    kernel_pmap.page_size = mmu_is_4kb_granule_supported() ? _4KB : (mmu_is_16kb_granule_supported() ? _16KB : _64KB);

    kernel_pmap.page_shift = _ctz(kernel_pmap.page_size);
    kernel_pmap.is_kernel = true;

    // Set the start and end of the kernel's virtual and physical address space
    // kernel_virtual_end is set to 0 at boot so that early bootstrap allocaters will kassert if they are called before pmap_init
    size_t kernel_size = (uintptr_t)(&__kernel_virtual_end) - (uintptr_t)(&__kernel_virtual_start);
    kernel_physical_end = kernel_physical_start + kernel_size;
    kernel_virtual_end = kernel_virtual_start + kernel_size;

    // Let's grab a page for the base translation table. This will need to mapped so bump up the physical and virtual end address
    // The base table for 16KB granule only has 2 entries while the 64KB granule base table only has 64 entries.
    kernel_physical_end = ROUND_PAGE_UP(kernel_pmap, kernel_physical_end);
    kernel_virtual_end = ROUND_PAGE_UP(kernel_pmap, kernel_virtual_end);
    kernel_pmap.ttb = kernel_physical_end;
    kernel_pmap.ttb_va = kernel_virtual_end;
    kernel_physical_end += kernel_pmap.page_size;
    kernel_virtual_end += kernel_pmap.page_size;
    kernel_size = kernel_virtual_end - kernel_virtual_start;

    // Zero the TTB
    memset((void*)kernel_pmap.ttb, 0, kernel_pmap.page_size);

    // We also need 1 L1/L2 table for the 16KB and 64KB page granules so we can use the recursive page mapping trick to
    // get access to the page tables through a known VA region (except for the TTB)
    // 4KB page granules can just add the recursive page table in the TTB and will be able to access the TTB through this region
    size_t max_num_ptes_ttb = MAX_NUM_PTES_TTB(kernel_pmap);
    size_t max_num_ptes_ll = MAX_NUM_PTES_LL(kernel_pmap);
    pte_t *ttb = (pte_t*)kernel_pmap.ttb;

    // This is used to keep track of physical pages allocated for page tables that we need to later mark off as being used via pmm_reserve
    paddr_t next_unused_pa = kernel_physical_end;

    // Attributes for the kernel's table descriptors
    t_attr_t t_attr_table = (t_attr_t){.ns = T_NON_SECURE, .ap = T_AP_NO_EL0, .uxn = T_UXN, .pxn = T_NON_PXN};
    bp_lattr_t bp_lattr_table = (bp_lattr_t){.ng = BP_GLOBAL, .af = BP_AF, .sh = BP_ISH, .ap = BP_AP_RW_NO_EL0, .ns = BP_NON_SECURE, .ma = BP_MA_NORMAL_WBWARA};

    if (kernel_pmap.page_size == _16KB || kernel_pmap.page_size == _64KB) {
        // Zero out the table
        memset((void*)next_unused_pa, 0, kernel_pmap.page_size);

        // Point the last entry in the TTB to this new table and then point the last entry in that table to itself
        ttb[max_num_ptes_ttb - 1] = MAKE_TDE(kernel_pmap, next_unused_pa, t_attr_table, bp_lattr_table);
        ((pte_t*)next_unused_pa)[max_num_ptes_ll - 1] = MAKE_TDE(kernel_pmap, next_unused_pa, t_attr_table, bp_lattr_table);

        next_unused_pa += kernel_pmap.page_size;
    } else {
        ttb[max_num_ptes_ttb - 1] = MAKE_TDE(kernel_pmap, kernel_pmap.ttb, t_attr_table, bp_lattr_table);
    }

    // Map the kernel's virtual address space to it's physical location. next_unused_pa is used by this routine to allocate page tables
    next_unused_pa = _pmap_init_map_range(kernel_virtual_start, kernel_physical_start, kernel_size, next_unused_pa);

    // Now let's create temporary mappings to identity map the kernel's physical address space (needed when we enable the MMU)
    // We need to allocate a new TTB since these mappings will be in TTBR0 while the kernel virtual mappings are in TTBR1
    // Don't update next_unused_pa because we are going to throw away these tables after the MMU is enabled
    paddr_t ttb0 = next_unused_pa;
    paddr_t ttb1 = kernel_pmap.ttb;
    kernel_pmap.ttb = ttb0;

    // FIXME: temporary mapping UART
    paddr_t _next_unused_pa = _pmap_init_map_range(kernel_physical_start, kernel_physical_start, kernel_size, next_unused_pa + kernel_pmap.page_size);
    _pmap_init_map_range(0x09000000, 0x09000000, kernel_pmap.page_size, _next_unused_pa);

    // Restore TTBR1 in the kernel's pmap and Create MAIR
    kernel_pmap.ttb = ttb1;
    ma_index_t ma_index = {.attrs = {MA_DEVICE_NGNRNE, MA_DEVICE_NGNRE, MA_NORMAL_NC, MA_NORMAL_INC, MA_NORMAL_WBWARA, MA_NORMAL_WTWARA, MA_NORMAL_WTWNRA, MA_NORMAL_WTWNRN}};

    // Finally enable the MMU!
    mmu_enable(ttb0, ttb1, MAIR(ma_index), kernel_pmap.page_size);

    // Perform stack adjustment and jump! And then disable TTBR0

    // Initialize pmm
    pmm_init();

    // Reserve the pages used by the kernel. The PMM works on the assumption of 4KB pages so we need to make sure we reserve the correct amount of pages
    for(unsigned long i = 0, num_pages = (next_unused_pa - kernel_physical_start) >> PMM_PAGE_SHIFT; i < num_pages; i++) {
        kprintf("pmm_reserve(%p)\n", kernel_physical_start + (i << PMM_PAGE_SHIFT));
        //pmm_reserve(kernel_physical_start + (i * PMM_PAGE_SIZE));
    }

    // Finally increment the reference count on the pmap. The refcount for kernel_pmap should never be 0.
    pmap_reference(pmap_kernel());
}

void pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp) {
    // The kernel's virtual address space ends where the page table VA space starts
    if(vstartp != NULL) *vstartp = kernel_virtual_start;
    if(vendp != NULL) *vendp = PT_IDX_TO_VA(kernel_pmap, 0);
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
