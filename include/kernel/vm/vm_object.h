#ifndef __VM_OBJECT_H__
#define __VM_OBJECT_H__

#include <sys/types.h>
#include <kernel/spinlock.h>
#include <kernel/list.h>
#include <kernel/arch/atomic.h>
#include <kernel/vm/vm_types.h>

// A virtual memory object represents any thing that can be allocated and referenced in a virtual address space
// An object can be mapped in multiple virtual address maps (i.e. shared) and may not be completely resident in
// memory. Objects can be backed by actual files or swap space if they are "anonymous" (i.e. not backed by anything)
typedef struct {
    spinlock_t lock;
    list_t ll_resident;  // List of resident pages for this object
    atomic_t refcnt;     // How many VM regions are referencing this object
    size_t size;         // Size of the object
} vm_object_t;

// All wired kernel memory belongs to this object
extern vm_object_t kernel_object;

// The linear mapped KVA space (not including the kernel code/data area) belongs to this object
extern vm_object_t kernel_lva_object;

// Initializes the vm_object module
void vm_object_init(void);

#endif // __VM_OBJECT_H__
