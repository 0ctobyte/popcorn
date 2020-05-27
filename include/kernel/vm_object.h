#ifndef __VM_OBJECT_H__
#define __VM_OBJECT_H__

#include <sys/types.h>

#include <kernel/vm_page.h>
#include <kernel/atomic.h>

// A virtual memory object represents any thing that can be allocated and referenced in a virtual address space
// An object can be mapped in multiple virtual address maps (i.e. shared) and may not be completely resident in
// memory. Objects can be backed by actual files or swap space if they are "anonymous" (i.e. not backed by anything)
typedef struct {
    vm_page_t *resident_pages; // List of resident pages for this object
    atomic_t refcnt;           // How many VM regions are referencing this object
} vm_object_t;

#endif // __VM_OBJECT_H__
