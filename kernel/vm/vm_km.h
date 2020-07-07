#ifndef _VM_KM_H_
#define _VM_KM_H_

#include <sys/types.h>
#include <kernel/vm/vm_types.h>

// Kernel virtual address space allocator. This module allows allocating/freeing virtual address space in the kernel's virtual memory map

typedef unsigned long vm_km_flags_t;

#define VM_KM_FLAGS_WIRED   (1)  // Request pages to be wired
#define VM_KM_FLAGS_VAONLY  (2)  // Don't allocate and map pages, only create the VA mapping in the map
#define VM_KM_FLAGS_ZERO    (4)  // Request that the allocated memory be zero-filled
#define VM_KM_FLAGS_EXEC    (8)  // Request that the allocated memory have executable rights
#define VM_KM_FLAGS_CANFAIL (16) // The operation is allowed to fail

// Initializes the Kernel's virtual memory map
void vm_km_init(void);

// Allocate virtual address space of the given size and flags
// If VM_KM_FLAGS_CANFAIL is set in flags then this function returns 0 on failure
vaddr_t vm_km_alloc(size_t size, vm_km_flags_t flags);

// Frees the virtual address range allocated by vm_km_alloc. va must be the same as that returned by vm_km_alloc.
// size and flags must be the same as the ones used in vm_km_alloc
void vm_km_free(vaddr_t va, size_t size, vm_km_flags_t flags);

#endif // _VM_KM_H_
