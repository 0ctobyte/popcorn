#ifndef __DEVICETREE_H__
#define __DEVICETREE_H__

#include <sys/types.h>

// Returns the base address and size of memory. Returns false if it could not find memory information in the FDT
bool devicetree_find_memory(unsigned long *base_addr, unsigned long *size);
bool devicetree_find_uart(void);

#endif // __DEVICETREE_H__
