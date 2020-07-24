/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _VM_TYPES_H_
#define _VM_TYPES_H_

#include <sys/types.h>

extern unsigned long MEMBASEADDR;
extern unsigned long MEMSIZE;
extern unsigned long PAGESIZE;
extern unsigned long PAGESHIFT;

#define IS_WITHIN_MEM_BOUNDS(addr)  (((addr) >= MEMBASEADDR) && ((addr) < (MEMBASEADDR + MEMSIZE)))
#define IS_PAGE_ALIGNED(addr)       (((long)(addr) & ((long)(PAGESIZE) - 1l)) == 0)
#define ROUND_PAGE_DOWN(addr)       ((long)(addr) & ~((long)((PAGESIZE) - 1l)))
#define ROUND_PAGE_UP(addr)         (IS_PAGE_ALIGNED(addr) ? (long)(addr) : ROUND_PAGE_DOWN(addr) + (long)(PAGESIZE))
#define KVA_TO_PA(kva)              (((kva) - kernel_virtual_start) + kernel_physical_start)
#define PA_TO_KVA(pa)               (((pa) - kernel_physical_start) + kernel_virtual_start)

// Virtual address
typedef uintptr_t vaddr_t;
extern vaddr_t max_kernel_virtual_end;
extern vaddr_t kernel_virtual_start;
extern vaddr_t kernel_virtual_end;

// Physical address
typedef uintptr_t paddr_t;
extern vaddr_t kernel_physical_start;
extern vaddr_t kernel_physical_end;

typedef unsigned long vm_offset_t;

// Protection bits
typedef unsigned long vm_prot_t;

#define VM_PROT_NONE    (0x0)
#define VM_PROT_READ    (0x1)
#define VM_PROT_WRITE   (0x2)
#define VM_PROT_EXECUTE (0x4)
#define VM_PROT_DEFAULT (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL     (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)

#endif // _VM_TYPES_H_
