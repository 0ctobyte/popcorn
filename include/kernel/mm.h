#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include <sys/types.h>

extern unsigned long MEMBASEADDR;
extern unsigned long MEMSIZE;

// Virtual address
typedef uintptr_t vaddr_t;

// Physical address
typedef uintptr_t paddr_t;

// Protection bits
typedef unsigned long vmm_prot_t;

typedef struct {
    // The starting virtual address of the page
    vaddr_t vaddr;
} vmm_page_t;

#define VM_PROT_NONE    (0x0)
#define VM_PROT_READ    (0x1)
#define VM_PROT_WRITE   (0x2)
#define VM_PROT_EXECUTE (0x4)
#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL     (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

#endif // __KERNEL_MM_H__
