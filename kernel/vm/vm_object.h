/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _VM_OBJECT_H_
#define _VM_OBJECT_H_

#include <sys/types.h>
#include <kernel/lock.h>
#include <kernel/list.h>
#include <kernel/vm/vm_types.h>

// A virtual memory object represents any thing that can be allocated and referenced in a virtual address space
// An object can be mapped in multiple virtual address maps (i.e. shared) and may not be completely resident in
// memory. Objects can be backed by actual files or swap space if they are "anonymous" (i.e. not backed by anything)
typedef struct {
    lock_t lock;          // RW lock
    list_t ll_resident;   // List of resident pages for this object
    unsigned long refcnt; // How many VM regions are referencing this object
    size_t size;          // Size of the object
} vm_object_t;

// All wired kernel memory belongs to this object
extern vm_object_t kernel_object;

// The linear mapped KVA space (not including the kernel code/data area) belongs to this object
extern vm_object_t kernel_lva_object;

// Initializes the vm_object module
void vm_object_init(void);

// Decrements the reference count; if it's zero frees the object and all it's pages
void vm_object_destroy(vm_object_t *object);

// Increment the reference count on the object
void vm_object_reference(vm_object_t *object);

// Set the new size of the given object. Only used for anonymous memory objects
void vm_object_set_size(vm_object_t *object, size_t new_size);

#endif // _VM_OBJECT_H_
