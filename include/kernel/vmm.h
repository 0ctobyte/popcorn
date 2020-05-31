#ifndef __VMM_H__
#define __VMM_H__

#include <sys/types.h>

extern unsigned long MEMBASEADDR;
extern unsigned long MEMSIZE;
extern unsigned long PAGESIZE;
extern unsigned long PAGESHIFT;

// Virtual address
typedef uintptr_t vaddr_t;
extern vaddr_t max_kernel_virtual_end;

// Physical address
typedef uintptr_t paddr_t;

// Protection bits
typedef unsigned long vm_prot_t;

typedef unsigned long vm_offset_t;

#define VM_PROT_NONE    (0x0)
#define VM_PROT_READ    (0x1)
#define VM_PROT_WRITE   (0x2)
#define VM_PROT_EXECUTE (0x4)
#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL     (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

void vmm_init(void);

#endif // __VMM_H__
