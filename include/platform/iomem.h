#ifndef __IOMEM_H__
#define __IOMEM_H__

#include <stdint.h>

#if defined(BBB)
#include <platform/beagleboneblack/iomem.h>
#elif defined(VIRT)
#include <platform/virt/iomem.h>
#else
#error "Undefined platform"
#endif

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

#endif // __IOMEM_H__

