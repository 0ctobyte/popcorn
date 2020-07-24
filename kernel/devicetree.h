/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _DEVICETREE_H_
#define _DEVICETREE_H_

#include <sys/types.h>

// Returns the base address and size of memory. Returns false if it could not find memory information in the FDT
bool devicetree_find_memory(unsigned long *base_addr, unsigned long *size);
bool devicetree_find_uart(void);

#endif // _DEVICETREE_H_
