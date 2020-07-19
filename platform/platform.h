#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <kernel/fdt.h>

// This is invoked before the virtual memory system is setup
void platform_early_init(fdt_header_t *fdth);

// This is invoked for further initialization after the virtual memory system is setup
void platform_init(fdt_header_t *fdth);

#endif // _PLATFORM_H_
