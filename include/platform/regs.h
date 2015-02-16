#ifndef __REG_H__
#define __REG_H__

#include <stdint.h>

#if defined(REALVIEW_PB)
  #include <platform/realview-pb/hwregs.h>
#elif defined(VERSATILE_PB)
  #include <platform/versatile-pb/hwregs.h>
#else
  #error "Undefined platform"
#endif

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

#endif // __REG_H__

