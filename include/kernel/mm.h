#ifndef __MM_H__
#define __MM_H__

#include <sys/types.h>

extern unsigned long MEMBASEADDR;
extern unsigned long MEMSIZE;
extern unsigned long PAGESIZE;
extern unsigned long PAGESHIFT;

// Virtual address
typedef uintptr_t vaddr_t;

// Physical address
typedef uintptr_t paddr_t;

// Protection bits
typedef unsigned long vm_prot_t;

#define VM_PROT_NONE    (0x0)
#define VM_PROT_READ    (0x1)
#define VM_PROT_WRITE   (0x2)
#define VM_PROT_EXECUTE (0x4)
#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL     (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

#endif // __MM_H__
