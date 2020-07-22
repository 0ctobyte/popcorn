#ifndef _ARM_GICV2_CPU_IF_H_
#define _ARM_GICV2_CPU_IF_H_

#include "arm_gicv2_common.h"

#define GICC_CTRL                                     (0x0000)
#define G_GICC_CTRL_EOIMODENS(r)                      (((r) >> 9) & 1)
#define G_GICC_CTRL_IRQBYPDISGRP1(r)                  (((r) >> 6) & 1)
#define G_GICC_CTRL_FIQBYPDISGRP1(r)                  (((r) >> 5) & 1)
#define G_GICC_CTRL_ENABLEGRP1(r)                     ((r) & 1)
#define S_GICC_CTRL_EOIMODENS                         (1 << 9)
#define S_GICC_CTRL_IRQBYPDISGRP1                     (1 << 6)
#define S_GICC_CTRL_FIQBYPDISGRP1                     (1 << 5)
#define S_GICC_CTRL_ENABLEGRP1                        (1)
#define gicc_ctrl_read(b)                             (MMIO_READ_32(b, GICC_CTRL))
#define gicc_ctrl_write(b, r)                         (MMIO_WRITE_32(b, GICC_CTRL, r))

#define GICC_PMR                                      (0x0004)
#define G_GICC_PMR_PRIORITY(r)                        ((r) & 0xff)
#define F_GICC_PMR_PRIORITY(f)                        ((f) & 0xff)
#define gicc_pmr_read(b)                              (MMIO_READ_32(b, GICC_PMR))
#define gicc_pmr_write(b, r)                          (MMIO_WRITE_32(b, GICC_PMR, r))

#define GICC_BPR                                      (0x0008)
#define G_GICC_BPR_PRIORITY(r)                        ((r) & 0x7)
#define F_GICC_BPR_PRIORITY(f)                        ((f) & 0x7)
#define gicc_bpr_read(b)                              (MMIO_READ_32(b, GICC_BPR))
#define gicc_bpr_write(b, r)                          (MMIO_WRITE_32(b, GICC_BPR, r))

#define GICC_IAR                                      (0x000C)
#define G_GICC_IAR_CPUID(r)                           (((r) >> 10) & 0x7)
#define G_GICC_IAR_INTID(r)                           ((r) & 0xffffff)
#define gicc_iar_read(b)                              (MMIO_READ_32(b, GICC_IAR))

#define GICC_EOIR                                     (0x0010)
#define F_GICC_EOIR_CPUID(r)                          (((r) & 0x7) << 10)
#define F_GICC_EOIR_INTID(r)                          ((r) & 0xffffff)
#define gicc_eoir_write(b, r)                         (MMIO_WRITE_32(b, GICC_EOIR, r))

#define GICC_RPR                                      (0x0014)
#define G_GICC_RPR_PRIORITY(r)                        ((r) & 0xff)
#define gicc_rpr_read(b)                              (MMIO_READ_32(b, GICC_RPR))

#define GICC_HPPIR                                    (0x0018)
#define G_GICC_HPPIR_CPUID(r)                         (((r) >> 10) & 0x7)
#define G_GICC_HPPIR_PENDINTID(r)                     ((r) & 0xffffff)
#define gicc_hppir_read(b)                            (MMIO_READ_32(b, GICC_HPPIR))

#define GICC_ABPR                                     (0x001C)
#define G_GICC_ABPR_PRIORITY(r)                       ((r) & 0x7)
#define F_GICC_ABPR_PRIORITY(f)                       ((f) & 0x7)
#define gicc_abpr_read(b)                             (MMIO_READ_32(b, GICC_ABPR))
#define gicc_abpr_write(b, r)                         (MMIO_WRITE_32(b, GICC_ABPR, r))

#define GICC_AIAR                                     (0x0020)
#define G_GICC_AIAR_CPUID(r)                          (((r) >> 10) & 0x7)
#define G_GICC_AIAR_INTID(r)                          ((r) & 0xffffff)
#define gicc_aiar_read(b)                             (MMIO_READ_32(b, GICC_AIAR))

#define GICC_AEOIR                                    (0x0024)
#define F_GICC_AEOIR_CPUID(r)                         (((r) & 0x7) << 10)
#define F_GICC_AEOIR_INTID(r)                         ((r) & 0xffffff)
#define gicc_aeoir_write(b, r)                        (MMIO_WRITE_32(b, GICC_AEOIR, r))

#define GICC_AHPPIR                                   (0x0028)
#define G_GICC_AHPPIR_CPUID(r)                        (((r) >> 10) & 0x7)
#define G_GICC_AHPPIR_PENDINTID(r)                    ((r) & 0xffffff)
#define gicc_ahppir_read(b)                           (MMIO_READ_32(b, GICC_AHPPIR))

#define GICC_APR(n)                                   (0x00D0 + (4 * (n)))
#define gicc_apr_read(b, id)                          (MMIO_READ_32(b, GICC_APR(id / 32))

#define GICC_NSAPR(n)                                 (0x00E0 + (4 * (n)))
#define gicc_apr_read(b, id)                          (MMIO_READ_32(b, GICC_APR(id / 32))

#define GICC_IIDR                                     (0x00FC)
#define G_GICC_IIDR_PRODUCTID(r)                      (((r) >> 20) & 0xfff)
#define G_GICC_IIDR_ARCHITECTURE_VERSION(r)           (((r) >> 16) & 0xf)
#define G_GICC_IIDR_REVISION(r)                       (((r) >> 12) & 0xf)
#define G_GICC_IIDR_IMPLEMENTER(r)                    ((r) & 0xfff)
#define gicc_iidr_read(b)                             (MMIO_READ_32(b, GICC_IIDR)

#define GICC_DIR                                      (0x1000)
#define F_GICC_DIR_CPUID(r)                           (((r) >> 10) & 0x7)
#define F_GICC_DIR_INTID(r)                           ((r) & 0xffffff)
#define gicc_dir_write(b, r)                          (MMIO_WRITE_32(b, GICC_DIR, r))

#endif // _ARM_GICV2_CPU_IF_H_
