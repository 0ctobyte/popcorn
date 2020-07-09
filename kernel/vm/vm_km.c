#include <kernel/kassert.h>
#include <kernel/arch/arch_asm.h>
#include <kernel/arch/pmap.h>
#include <kernel/vm/vm_types.h>
#include <kernel/vm/vm_map.h>
#include <kernel/vm/vm_km.h>

extern paddr_t kernel_physical_start;
extern paddr_t kernel_physical_end;

void vm_km_init(void) {
    vaddr_t kernel_virtual_start, kernel_virtual_end;
    pmap_virtual_space(&kernel_virtual_start, &kernel_virtual_end);

    // Initialize the kernel vm_map
    kernel_vmap = (vm_map_t){ .lock = SPINLOCK_INIT, .pmap = pmap_kernel(), .start = kernel_virtual_start, .end = max_kernel_virtual_end, .size = 0, .refcnt = 0 };

    // Add a mapping to the kernel_object for the kernel code/data area
    size_t size = kernel_physical_end - kernel_physical_start;
    kresult_t res = vm_map_enter_at(vm_map_kernel(), kernel_virtual_start, size, &kernel_object, 0, VM_PROT_ALL);
    kassert(res == KRESULT_OK);

    // Add a mapping to the linear mapped KVA space
    res = vm_map_enter_at(vm_map_kernel(), kernel_virtual_start + size, MEMSIZE - size, &kernel_lva_object, 0, VM_PROT_DEFAULT);
    kassert(res == KRESULT_OK);
}

vaddr_t vm_km_alloc(size_t size, vm_km_flags_t flags) {
    vaddr_t vstart;
    vm_prot_t prot = (flags & VM_KM_FLAGS_EXEC) ? VM_PROT_ALL : VM_PROT_DEFAULT;
    vm_offset_t offset = kernel_object.size;

    size = ROUND_PAGE_UP(size);

    // Enter a mapping for this virtual address range
    kresult_t res = vm_map_enter(vm_map_kernel(), &vstart, size, &kernel_object, kernel_object.size, prot);

    if (res != KRESULT_OK) {
        if (flags & VM_KM_FLAGS_CANFAIL) {
            return 0;
        } else {
            panic("vm_km_alloc - vm_map_enter fail");
        }
    }

    // We're done here if we only wanted a VA mapping and no actualy physical pages backing it
    if (flags & VM_KM_FLAGS_VAONLY) return vstart;

    // If all goes well, allocate pages into the kernel object for this mapping and enter it into the pmap
    for (vaddr_t vaddr = vstart, vend = vstart + size; vaddr < vend; vaddr += PAGESIZE, offset += PAGESIZE) {
        vm_page_t *page = vm_page_alloc(&kernel_object, offset);

        pmap_flags_t flags = PMAP_FLAGS_WRITE_BACK;
        if (flags & VM_KM_FLAGS_WIRED) {
            flags |= PMAP_FLAGS_WIRED;
        }

        if (flags & VM_KM_FLAGS_CANFAIL) {
            flags |= PMAP_FLAGS_CANFAIL;
        }

        res = pmap_enter(pmap_kernel(), vaddr, vm_page_to_pa(page), prot, flags);

        if (res != 0) {
            if (flags & VM_KM_FLAGS_CANFAIL) {
                return 0;
            } else {
                panic("vm_km_alloc - pmap_enter fail");
            }
        }
    }

    // Finally zero out the pages if required
    if (flags & VM_KM_FLAGS_ZERO) arch_fast_zero((void*)vstart, size);

    return vstart;
}

void vm_km_free(vaddr_t va, size_t size, vm_km_flags_t flags) {
}
