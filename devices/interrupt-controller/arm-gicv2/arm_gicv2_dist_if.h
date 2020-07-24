/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _ARM_GICV2_DIST_IF_H_
#define _ARM_GICV2_DIST_IF_H_

#include "arm_gicv2_common.h"

#define GICD_CTRL                                     (0x0000)
#define G_GICD_CTRL_ENABLE(r)                         ((r) & 0x1)
#define S_GICD_CTRL_ENABLE                            (1)
#define gicd_ctrl_read(b)                             (MMIO_READ_32(b, GICD_CTRL))
#define gicd_ctrl_write(b, r)                         (MMIO_WRITE_32(b, GICD_CTRL, r))

#define GICD_TYPER                                    (0x0004)
#define G_GICD_TYPER_LSPI(r)                          (((r) >> 11) & 0x1f)
#define G_GICD_TYPER_SECURITYEXTN(r)                  (((r) >> 10) & 0x1)
#define G_GICD_TYPER_CPUNUMBER(r)                     (((r) >> 5) & 0x7)
#define G_GICD_TYPER_ITLINESNUMBER(r)                 (32 * (((r) & 0x1f) + 1))
#define gicd_typer_read(b)                            (MMIO_READ_32(b, GICD_TYPER))

#define GICD_IIDR                                     (0x0008)
#define G_GICD_IIDR_PRODUCTID(r)                      (((r) >> 24) & 0xff)
#define G_GICD_IIDR_VARIANT(r)                        (((r) >> 16) & 0xf)
#define G_GICD_IIDR_REVISION(r)                       (((r) >> 12) & 0xf)
#define G_GICD_IIDR_IMPLEMENTER(r)                    ((r) & 0xfff)
#define gicd_iidr_read(b)                             (MMIO_READ_32(b, GICD_IIDR)

#define GICD_IGROUPR(n)                               (0x0080 + (4 * (n)))
#define gicd_igroupr_set(b, id)                       GIC_SET_BIT_RMW_32(b, GICD_IGROUPR, id)
#define gicd_igroupr_clr(b, id)                       GIC_CLR_BIT_RMW_32(b, GICD_IGROUPR, id)

#define GICD_ISENABLER(n)                             (0x0100 + (4 * (n)))
#define gicd_isenabler_set(b, id)                     GIC_SET_BIT_32(b, GICD_ISENABLER, id)

#define GICD_ICENABLER(n)                             (0x0180 + (4 * (n)))
#define gicd_icenabler_set(b, id)                     GIC_SET_BIT_32(b, GICD_ICENABLER, id)

#define GICD_ISPENDR(n)                               (0x0200 + (4 * (n)))
#define gicd_ispendr_set(b, id)                       GIC_SET_BIT_32(b, GICD_ISPENDR, id)

#define GICD_ICPENDR(n)                               (0x0280 + (4 * (n)))
#define gicd_icpendr_set(b, id)                       GIC_SET_BIT_32(b, GICD_ICPENDR, id)

#define GICD_ISACTIVER(n)                             (0x0300 + (4 * (n)))
#define gicd_isactiver_set(b, id)                     GIC_SET_BIT_32(b, GICD_ISACTIVER, id)

#define GICD_ICACTIVER(n)                             (0x0380 + (4 * (n)))
#define gicd_icactiver_set(b, id)                     GIC_SET_BIT_32(b, GICD_ICACTIVER, id)

#define GICD_IPRIORITYR(n)                            (0x0400 + (4 * (n)))
#define gicd_ipriorityr_set(b, id, v)                 GIC_SET_FLD_RMW_32(b, GICD_IPRIORITYR, 8, id, v)
#define gicd_ipriorityr_clr(b, id)                    GIC_CLR_FLD_RMW_32(b, GICD_IPRIORITYR, 8, id, 0xff)

#define GICD_ITARGETSR(n)                             (0x0800 + (4 * (n)))
#define gicd_itargetsr_set(b, id, v)                  GIC_SET_FLD_RMW_32(b, GICD_ITARGETSR, 8, id, v)
#define gicd_itargetsr_clr(b, id)                     GIC_CLR_FLD_RMW_32(b, GICD_ITARGETSR, 8, id, 0xff)

#define GICD_ICFGR(n)                                 (0x0C00 + (4 * (n)))
#define gicd_icfgr_set(b, id)                         GIC_SET_FLD_RMW_32(b, GICD_ICFGR, 2, id, 0x2)
#define gicd_icfgr_clr(b, id)                         GIC_CLR_FLD_RMW_32(b, GICD_ICFGR, 2, id, 0x3)

#define GICD_NSACR(n)                                 (0x0E00 + (4 * (n)))
#define gicd_nsacr_set(b, id)                         GIC_SET_BIT_RMW_32(b, GICD_NSACR, id)
#define gicd_nsacr_clr(b, id)                         GIC_CLR_BIT_RMW_32(b, GICD_NSACR, id)

#define GICD_SGIR                                     (0x0F00)
#define F_GICD_SGIR_TARGETLISTFILTER(f)               (((f) & 0x3) << 24)
#define F_GICD_SGIR_CPUTARGETLIST(f)                  (((f) & 0xff) << 16)
#define S_GICD_SGIR_NSATT                             (1 << 15)
#define F_GICD_SGIR_INTID(f)                          ((f) & 0xf)
#define gicd_sgir_write(b, r)                         (MMIO_WRITE_32(b, GICD_SGIR))

#define GICD_CPENDSGIR(n)                             (0x0F10 + (4 * (n)))
#define gicd_cpendsgir_set(b, id)                     GIC_SET_BIT_32(b, GICD_CPENDSGIR, id)

#define GICD_SPENDSGIR(n)                             (0x0F20 + (4 * (n)))
#define gicd_SPENDSGIr_set(b, id)                     GIC_SET_BIT_32(b, GICD_SPENDSGIR, id)

#endif // _ARM_GICV2_DIST_IF_H_
