/* 
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _ARM_GICV3_DIST_IF_H_
#define _ARM_GICV3_DIST_IF_H_

#include "arm_gicv3_common.h"

#define GICD_CTRL                                     (0x0000)
#define G_GICD_CTRL_RWP(r)                            (((r) >> 31) & 0x1)
#define G_GICD_CTRL_ARE_NS(r)                         (((r) >> 4) & 0x1)
#define G_GICD_CTRL_ENABLEGRP1(r)                     (((r) >> 1) & 0x1)
#define G_GICD_CTRL_ENABLEGRP0(r)                     ((r) & 0x1)
#define S_GICD_CTRL_ARE                               (1 << 4)
#define S_GICD_CTRL_ENABLEGRP1                        (1 << 1)
#define S_GICD_CTRL_ENABLEGRP0                        (1)
#define gicd_ctrl_read(b)                             (MMIO_READ_32(b, GICD_CTRL))
#define gicd_ctrl_write(b, r)                         (MMIO_WRITE_32(b, GICD_CTRL, r))

#define GICD_TYPER                                    (0x0004)
#define G_GICD_TYPER_ESPI_RANGE(r)                    (32 * ((((r) >> 27) & 0x1f) + 1) + 4095)
#define G_GICD_TYPER_RSS(r)                           (((r) >> 26) & 0x1)
#define G_GICD_TYPER_NO1N(r)                          (((r) >> 25) & 0x1)
#define G_GICD_TYPER_A3V(r)                           (((r) >> 24) & 0x1)
#define G_GICD_TYPER_IDBITS(r)                        ((((r) >> 19) & 0x1f) + 1)
#define G_GICD_TYPER_DVIS(r)                          (((r) >> 18) & 0x1)
#define G_GICD_TYPER_LPIS(r)                          (((r) >> 17) & 0x1)
#define G_GICD_TYPER_MBIS(r)                          (((r) >> 16) & 0x1)
#define G_GICD_TYPER_NUM_LPIS(r)                      ( 1 << ((((r) >> 11) & 0x1f) + 1))
#define G_GICD_TYPER_SECURITYEXTN(r)                  (((r) >> 10) & 0x1)
#define G_GICD_TYPER_ESPI(r)                          (((r) >> 8) & 0x1)
#define G_GICD_TYPER_CPUNUMBER(r)                     (((r) >> 5) & 0x7)
#define G_GICD_TYPER_ITLINESNUMBER(r)                 (32 * (((r) & 0x1f) + 1) - 1)
#define gicd_typer_read(b)                            (MMIO_READ_32(b, GICD_TYPER))

#define GICD_IIDR                                     (0x0008)
#define G_GICD_IIDR_PRODUCTID(r)                      (((r) >> 24) & 0xff)
#define G_GICD_IIDR_VARIANT(r)                        (((r) >> 16) & 0xf)
#define G_GICD_IIDR_REVISION(r)                       (((r) >> 12) & 0xf)
#define G_GICD_IIDR_IMPLEMENTER(r)                    ((r) & 0xfff)
#define gicd_iidr_read(b)                             (MMIO_READ_32(b, GICD_IIDR)

#define GICD_TYPER2                                   (0x000C)
#define G_GICD_TYPER2_VIL(r)                          (((r) >> 7) & 0x1)
#define G_GICD_TYPER2_VID(r)                          ((r) & 0x1f)
#define gicd_typer2_read(b)                           (MMIO_READ_32(b, GICD_TYPER2))

#define GICD_STATUSR                                  (0x0010)
#define G_GICD_STATUSR_WROD(r)                        (((r) >> 3) & 0x1)
#define G_GICD_STATUSR_RWOD(r)                        (((r) >> 2) & 0x1)
#define G_GICD_STATUSR_WRD(r)                         (((r) >> 1) & 0x1)
#define G_GICD_STATUSR_RRD(r)                         ((r) & 0x1)
#define gicd_statusr_read(b)                          (MMIO_READ_32(b, GICD_STATUSR))

#define GICD_SETSPI_NSR                               (0x0040)
#define F_GICD_SETSPI_NSR_INTID(f)                    ((f) & 0x1fff)
#define gicd_setspi_nsr_write(b, r)                   (MMIO_WRITE_32(b, GICD_SETSPI_NSR, r))

#define GICD_CLRSPI_NSR                               (0x0048)
#define F_GICD_CLRSPI_NSR_INTID(f)                    ((f) & 0x1fff)
#define gicd_clrspi_nsr_write(b, r)                   (MMIO_WRITE_32(b, GICD_CLRSPI_NSR, r))

#define GICD_SETSPI_SR                                (0x0040)
#define F_GICD_SETSPI_SR_INTID(f)                     ((f) & 0x1fff)
#define gicd_setspi_sr_write(b, r)                    (MMIO_WRITE_32(b, GICD_SETSPI_SR, r))

#define GICD_CLRSPI_SR                                (0x0048)
#define F_GICD_CLRSPI_SR_INTID(f)                     ((f) & 0x1fff)
#define gicd_clrspi_sr_write(b, r)                    (MMIO_WRITE_32(b, GICD_CLRSPI_SR, r))

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

#define GICD_IGRPMODR(n)                              (0x0D00 + (4 * (n)))
#define gicd_igrpmodr_set(b, id)                      GIC_SET_BIT_RMW_32(b, GICD_IGRPMODR, id)
#define gicd_igrpmodr_clr(b, id)                      GIC_CLR_BIT_RMW_32(b, GICD_IGRPMODR, id)

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

#define GICD_IGROUPRE(n)                              (0x1000 + (4 * (n)))
#define gicd_igroupre_set(b, id)                      GIC_SET_BIT_RMW_32(b, GICD_IGROUPRE, id)
#define gicd_igroupre_clr(b, id)                      GIC_CLR_BIT_RMW_32(b, GICD_IGROUPRE, id)

#define GICD_ISENABLERE(n)                            (0x1200 + (4 * (n)))
#define gicd_isenablere_set(b, id)                    GIC_SET_BIT_32(b, GICD_ISENABLERE, id)

#define GICD_ICENABLERE(n)                            (0x1400 + (4 * (n)))
#define gicd_icenablere_set(b, id)                    GIC_SET_BIT_32(b, GICD_ICENABLERE, id)

#define GICD_ISPENDRE(n)                              (0x1600 + (4 * (n)))
#define gicd_ispendre_set(b, id)                      GIC_SET_BIT_32(b, GICD_ISPENDRE, id)

#define GICD_ICPENDRE(n)                              (0x1800 + (4 * (n)))
#define gicd_icpendre_set(b, id)                      GIC_SET_BIT_32(b, GICD_ICPENDRE, id)

#define GICD_ISACTIVERE(n)                            (0x1A00 + (4 * (n)))
#define gicd_isactivere_set(b, id)                    GIC_SET_BIT_32(b, GICD_ISACTIVERE, id)

#define GICD_ICACTIVERE(n)                            (0x1C00 + (4 * (n)))
#define gicd_icactivere_set(b, id)                    GIC_SET_BIT_32(b, GICD_ICACTIVERE, id)

#define GICD_IPRIORITYRE(n)                           (0x2000 + (4 * (n)))
#define gicd_ipriorityre_set(b, id, v)                GIC_SET_FLD_RMW_32(b, GICD_IPRIORITYRE, 8, id, v)
#define gicd_ipriorityre_clr(b, id)                   GIC_CLR_FLD_RMW_32(b, GICD_IPRIORITYRE, 8, id, 0xff)

#define GICD_ICFGRE(n)                                (0x3000 + (4 * (n)))
#define gicd_icfgre_set(b, id)                        GIC_SET_FLD_RMW_32(b, GICD_ICFGRE, 2, id, 0x2)
#define gicd_icfgre_clr(b, id)                        GIC_CLR_FLD_RMW_32(b, GICD_ICFGRE, 2, id, 0x3)

#define GICD_IGRPMODRE(n)                             (0x3400 + (4 * (n)))
#define gicd_igrpmodre_set(b, id)                     GIC_SET_BIT_RMW_32(b, GICD_IGRPMODRE, id)
#define gicd_igrpmodre_clr(b, id)                     GIC_CLR_BIT_RMW_32(b, GICD_IGRPMODRE, id)

#define GICD_NSACRE(n)                                (0x3600 + (4 * (n)))
#define gicd_nsacre_set(b, id)                        GIC_SET_BIT_RMW_32(b, GICD_NSACRE, id)
#define gicd_nsacre_clr(b, id)                        GIC_CLR_BIT_RMW_32(b, GICD_NSACRE, id)

#define GICD_IROUTER(n)                               (0x6100 + (8 * (n)))
#define S_GICD_IROUTER_INTERRUPT_ROUTING_MODE         (1 << 31)
#define gicd_irouter_read(b, id)                      (MMIO_READ_64(b, GICD_IROUTER(id)))
#define gicd_irouter_write(b, id, v)                  (MMIO_WRITE_64(b, GICD_IROUTER(id), v))

#define GICD_IROUTERE(n)                              (0x8000 + (8 * (n)))
#define S_GICD_IROUTERE_INTERRUPT_ROUTING_MODE        (1 << 31)
#define gicd_iroutere_read(b, id)                     (MMIO_READ_64(b, GICD_IROUTERE(id)))
#define gicd_iroutere_write(b, id, v)                 (MMIO_WRITE_64(b, GICD_IROUTERE(id), v))

#endif // _ARM_GICV3_DIST_IF_H_
