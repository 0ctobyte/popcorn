#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include <sys/types.h>

#include <lib/asm.h>

extern unsigned long PAGESIZE;
extern unsigned long MEMBASEADDR;
extern unsigned long MEMSIZE;

// Virtual address
typedef uintptr_t vaddr_t;

// Physical address
typedef uintptr_t paddr_t;

// Protection bits
typedef unsigned long vm_prot_t;

#define VM_PROT_NONE (0x0)
#define VM_PROT_READ (0x1)
#define VM_PROT_WRITE (0x2)
#define VM_PROT_EXECUTE (0x4)
#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

// Should these even be here?
#define IS_PAGE_ALIGNED(B) (((B) & (PAGESIZE - 1)) == 0)
#define IS_WITHIN_BOUNDS(B) ((((B) >= MEMBASEADDR) && ((B) < (MEMBASEADDR+MEMSIZE))))
#define TRUNC_PAGE(B) ((B) & ~(PAGESIZE - 1))
#define ROUND_PAGE(B) ((IS_PAGE_ALIGNED((B))) ? (B) : (TRUNC_PAGE((B)) + PAGESIZE))
#define ATOP(B) (TRUNC_PAGE((B)) >> _ctz(PAGESIZE))
#define PTOA(B) ((B) << _ctz(PAGESIZE))

#endif // __KERNEL_MM_H__
