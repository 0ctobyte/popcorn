/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _ARM_GICV2_COMMON_H_
#define _ARM_GICV2_COMMON_H_

#include <sys/types.h>

#define MMIO_READ_32(base, reg)       (*((uint32_t*)((base) + (reg))))
#define MMIO_READ_64(base, reg)       (*((uint64_t*)((base) + (reg))))
#define MMIO_WRITE_32(base, reg, val) (*((uint32_t*)((base) + (reg))) = (val))
#define MMIO_WRITE_64(base, reg, val) (*((uint64_t*)((base) + (reg))) = (val))

#define GIC_SET_FLD_32(base, reg, nbits, id, v)\
({\
    unsigned int f = 32 / (nbits);\
    unsigned int n = (id) / f;\
    unsigned int val = (v) & ((1 << (nbits)) - 1);\
    MMIO_WRITE_32((base), reg(n), val << (((id) % f) * nbits));\
})

#define _GIC_SET_BIT_32(base, reg, id)              MMIO_WRITE_32(base, reg, 1 << (id))
#define _GIC_CLR_BIT_32(base, reg, id)              MMIO_WRITE_32(base, reg, 1 << (id))
#define GIC_SET_BIT_32(base, reg, id)               GIC_SET_FLD_32(base, reg, 1, id, 1)
#define GIC_CLR_BIT_32(base, reg, id)               GIC_SET_FLD_32(base, reg, 1, id, 1)

#define GIC_SET_FLD_RMW_32(base, reg, nbits, id, v)\
({\
    unsigned int f = 32 / (nbits);\
    unsigned int n = (id) / f;\
    unsigned int val = (v) & ((1 << (nbits)) - 1);\
    MMIO_WRITE_32((base), reg(n), MMIO_READ_32((base), reg(n)) | (val << (((id) % f) * nbits)));\
})

#define GIC_CLR_FLD_RMW_32(base, reg, nbits, id, v)\
({\
    unsigned int f = 32 / (nbits);\
    unsigned int n = (id) / f;\
    unsigned int val = (v) & ((1 << (nbits)) - 1);\
    MMIO_WRITE_32((base), reg(n), MMIO_READ_32((base), reg(n)) & ~(val << (((id) % f) * nbits)));\
})

#define _GIC_SET_BIT_RMW_32(base, reg, id)          MMIO_WRITE_32(base, reg, MMIO_READ_32(base, reg) | (1 << (id)))
#define _GIC_CLR_BIT_RMW_32(base, reg, id)          MMIO_WRITE_32(base, reg, MMIO_READ_32(base, reg) & ~(1 << (id)))
#define GIC_SET_BIT_RMW_32(base, reg, id)           GIC_SET_FLD_RMW_32(base, reg, 1, id, 1)
#define GIC_CLR_BIT_RMW_32(base, reg, id)           GIC_CLR_FLD_RMW_32(base, reg, 1, id, 1)

#endif // _ARM_GICV2_COMMON_H_
