/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _ARM_GICV3_RDIST_IF_H_
#define _ARM_GICV3_RDIST_IF_H_

#include "arm_gicv3_common.h"

#define GICR_CTLR                                     (0x0000)
#define G_GICR_CTLR_UWP(r)                            (((r) >> 31) & 1)
#define G_GICR_CTLR_RWP(r)                            (((r) >> 3) & 1)
#define G_GICR_CTLR_CES(r)                            (((r) >> 1) & 1)
#define S_GICR_CTLR_DPG1S                             (1 << 26)
#define S_GICR_CTLR_DPG1NS                            (1 << 25)
#define S_GICR_CTLR_DPG0                              (1 << 24)
#define S_GICR_CTLR_ENABLELPIS                        (1)
#define gicr_ctlr_read(b)                             (MMIO_READ_32(b, GICR_CTLR))
#define gicr_ctlr_write(b, r)                         (MMIO_WRITE_32(b, GICR_CTLR, r))

#define GICR_IIDR                                     (0x0004)
#define G_GICR_IIDR_PRODUCTID(r)                      (((r) >> 24) & 0xff)
#define G_GICR_IIDR_VARIANT(r)                        (((r) >> 16) & 0xf)
#define G_GICR_IIDR_REVISION(r)                       (((r) >> 12) & 0xf)
#define G_GICR_IIDR_IMPLEMENTER(r)                    ((r) & 0xfff)
#define gicr_iidr_read(b)                             (MMIO_READ_32(b, GICR_IIDR))

#define GICR_TYPER                                    (0x0008)
#define G_GICR_TYPER_AFFINITY_VALUE(r)                (((r) >> 32) & 0xffffffff)
#define G_GICR_TYPER_PPINUM(r)                        (((r) >> 27) & 0x1f)
#define G_GICR_TYPER_VSGI(r)                          (((r) >> 26) & 0x1)
#define G_GICR_TYPER_COMMONLPIAFF(r)                  (((r) >> 24) & 0x3)
#define G_GICR_TYPER_PROCESSOR_NUMBER(r)              (((r) >> 8) & 0xffff)
#define G_GICR_TYPER_RVPEID(r)                        (((r) >> 7) & 0x1)
#define G_GICR_TYPER_MPAM(r)                          (((r) >> 6) & 0x1)
#define G_GICR_TYPER_DPGS(r)                          (((r) >> 5) & 0x1)
#define G_GICR_TYPER_LAST(r)                          (((r) >> 4) & 0x1)
#define G_GICR_TYPER_DIRECTLPIS(r)                    (((r) >> 3) & 0x1)
#define G_GICR_TYPER_DIRTY(r)                         (((r) >> 2) & 0x1)
#define G_GICR_TYPER_VLPIS(r)                         (((r) >> 1) & 0x1)
#define G_GICR_TYPER_PLPIS(r)                         (((r) >> 0) & 0x1)
#define gicr_typer_read(b)                            (MMIO_READ_64(b, GICR_TYPER))

#define GICR_STATUSR                                  (0x0010)
#define G_GICR_STATUSR_WROD(r)                        (((r) >> 3) & 0x1)
#define G_GICR_STATUSR_RWOD(r)                        (((r) >> 2) & 0x1)
#define G_GICR_STATUSR_WRD(r)                         (((r) >> 1) & 0x1)
#define G_GICR_STATUSR_RRD(r)                         ((r) & 0x1)
#define gicr_statusr_read(b)                          (MMIO_READ_32(b, GICR_STATUSR))

#define GICR_WAKER                                    (0x0014)
#define G_GICR_WAKER_CHILDRENASLEEP(r)                (((r) >> 2) & 1)
#define G_GICR_WAKER_PROCESSORSLEEP(r)                (((r) >> 1) & 1)
#define S_GICR_WAKER_PROCESSORSLEEP                   (1 << 1)
#define gicr_waker_read(b)                            (MMIO_READ_32(b, GICR_WAKER))
#define gicr_waker_write(b, r)                        (MMIO_WRITE_32(b, GICR_WAKER, r))

#define GICR_MPAMIDR                                  (0x0018)
#define G_GICR_MPAMIDR_PMGMAX(r)                      (((r) >> 16) & 0xff)
#define G_GICR_MPAMIDR_PARTIDMAX(r)                   ((r) & 0xffff)
#define gicr_mpamidr_read(b)                          (MMIO_READ_32(b, GICR_MPAMIDR))

#define GICR_PARTIDR                                  (0x001C)
#define G_GICR_PARTIDR_PMG(r)                         (((r) >> 16) & 0xff)
#define G_GICR_PARTIDR_PARTID(r)                      ((r) & 0xffff)
#define gicr_partidr_read(b)                          (MMIO_READ_32(b, GICR_PARTIDR))

#define GICR_SETLPIR                                  (0x0040)
#define F_GICR_SETLPIR_INTID(f)                       ((f) & 0xffffffff)
#define gicr_setlpir_write(b, r)                      (MMIO_WRITE_64(b, GICR_SETLPIR, r))

#define GICR_CLRLPIR                                  (0x0048)
#define F_GICR_CLRLPIR_INTID(f)                       ((f) & 0xffffffff)
#define gicr_clrlpir_write(b, r)                      (MMIO_WRITE_64(b, GICR_CLRLPIR, r))

#define GICR_PROPBASER                                (0x0070)
#define F_GICR_PROPBASER_OUTERCACHE(f)                (((f) & 0x7) << 56)
#define F_GICR_PROPBASER_PHYSICAL_ADDRESS(f)          ((f) & 0xffffffffff000)
#define F_GICR_PROPBASER_SHAREABILITY(f)              (((f) & 0x3) << 10)
#define F_GICR_PROPBASER_INNERCACHE(f)                (((f) & 0x7) << 7)
#define F_GICR_PROPBASER_IDBITS(f)                    ((f) & 0x1f)
#define gicr_propbaser_read(b)                        (MMIO_WRITE_64(b, GICR_PROPBASER))
#define gicr_propbaser_write(b, r)                    (MMIO_WRITE_64(b, GICR_PROPBASER, r))

#define GICR_PENDBASER                                (0x0078)
#define S_GICR_PENDBASER_PTZ                          (1 << 62)
#define F_GICR_PENDBASER_OUTERCACHE(f)                (((f) & 0x7) << 56)
#define F_GICR_PENDBASER_PHYSICAL_ADDRESS(f)          ((f) & 0xfffffffff0000)
#define F_GICR_PENDBASER_SHAREABILITY(f)              (((f) & 0x3) << 10)
#define F_GICR_PENDBASER_INNERCACHE(f)                (((f) & 0x7) << 7)
#define gicr_pendbaser_read(b)                        (MMIO_WRITE_64(b, GICR_PENDBASER))
#define gicr_pendbaser_write(b, r)                    (MMIO_WRITE_64(b, GICR_PENDBASER, r))

#define GICR_INVLPIR                                  (0x00A0)
#define S_GICR_INVLPIR_V                              (1 << 63)
#define F_GICR_INVLPIR_VPEID(f)                       (((f) >> 32) & 0xffff)
#define F_GICR_INVLPIR_INTID(f)                       ((f) & 0xffffffff)
#define gicr_invlpir_write(b, r)                      (MMIO_WRITE_64(b, GICR_INVLPIR, r))

#define GICR_INVALLR                                  (0x00B0)
#define S_GICR_INVALLR_V                              (1 << 63)
#define F_GICR_INVALLR_VPEID(f)                       (((f) >> 32) & 0xffff)
#define gicr_invallr_write(b, r)                      (MMIO_WRITE_64(b, GICR_INVALLR, r))

#define GICR_SYNCR                                    (0x00C0)
#define G_GICR_SYNCR_BUSY(r)                          ((r) & 1)
#define gicr_syncr_read(b)                            (MMIO_READ_32(b, GICR_SYNCR))

#define GICR_VPROPBASER                               (0x20070)
#define F_GICR_VPROPBASER_OUTERCACHE(f)               (((f) & 0x7) << 56)
#define F_GICR_VPROPBASER_PHYSICAL_ADDRESS(f)         ((f) & 0xffffffffff000)
#define F_GICR_VPROPBASER_SHAREABILITY(f)             (((f) & 0x3) << 10)
#define F_GICR_VPROPBASER_INNERCACHE(f)               (((f) & 0x7) << 7)
#define F_GICR_VPROPBASER_IDBITS(f)                   ((f) & 0x1f)
#define gicr_vpropbaser_read(b)                       (MMIO_WRITE_64(b, GICR_VPROPBASER))
#define gicr_vpropbaser_write(b, r)                   (MMIO_WRITE_64(b, GICR_VPROPBASER, r))

#define GICR_VPENDBASER                               (0x20078)
#define S_GICR_VPENDBASER_VALID                       (1 << 63)
#define S_GICR_VPENDBASER_IDAI                        (1 << 62)
#define S_GICR_VPENDBASER_PENDINGLAST                 (1 << 61)
#define S_GICR_VPENDBASER_DIRTY                       (1 << 60)
#define F_GICR_VPENDBASER_OUTERCACHE(f)               (((f) & 0x7) << 56)
#define F_GICR_VPENDBASER_PHYSICAL_ADDRESS(f)         ((f) & 0xfffffffff0000)
#define F_GICR_VPENDBASER_SHAREABILITY(f)             (((f) & 0x3) << 10)
#define F_GICR_VPENDBASER_INNERCACHE(f)               (((f) & 0x7) << 7)
#define gicr_vpendbaser_read(b)                       (MMIO_WRITE_64(b, GICR_PENDBASER))
#define gicr_vpendbaser_write(b, r)                   (MMIO_WRITE_64(b, GICR_PENDBASER, r))

#define GICR_VSGIR                                    (0x20080)
#define F_GICR_VSGIR_VPEID(f)                         ((f) & 0xffff)
#define gicr_vsgir_write(b, r)                        (MMIO_WRITE_64(b, GICR_VSGIR, r))

#define GICR_VSGIPENDR                                (0x20088)
#define G_GICR_VSGIPENDR_BUSY(r)                      (((r) >> 31) & 1)
#define G_GICR_VSGIPENDR_PENDING(r)                   ((r) & 0xffff)
#define gicr_vsgipendr_write(b, r)                    (MMIO_WRITE_32(b, GICR_VSGIPENDR, r))

#define GICR_IGROUPR(n)                               (0x10080 + (4 * (n)))
#define gicr_igroupr_set(b, id)                       GIC_SET_BIT_RMW_32(b, GICR_IGROUPR, id)
#define gicr_igroupr_clr(b, id)                       GIC_CLR_BIT_RMW_32(b, GICR_IGROUPR, id)

#define GICR_ISENABLER(n)                             (0x10100 + (4 * (n)))
#define gicr_isenabler_set(b, id)                     GIC_SET_BIT_32(b, GICR_ISENABLER, id)

#define GICR_ICENABLER(n)                             (0x10180 + (4 * (n)))
#define gicr_icenabler_set(b, id)                     GIC_SET_BIT_32(b, GICR_ICENABLER, id)

#define GICR_ISPENDR(n)                               (0x10200 + (4 * (n)))
#define gicr_ispendr_set(b, id)                       GIC_SET_BIT_32(b, GICR_ISPENDR, id)

#define GICR_ICPENDR(n)                               (0x10280 + (4 * (n)))
#define gicr_icpendr_set(b, id)                       GIC_SET_BIT_32(b, GICR_ICPENDR, id)

#define GICR_ISACTIVER(n)                             (0x10300 + (4 * (n)))
#define gicr_isactiver_set(b, id)                     GIC_SET_BIT_32(b, GICR_ISACTIVER, id)

#define GICR_ICACTIVER(n)                             (0x10380 + (4 * (n)))
#define gicr_icactiver_set(b, id)                     GIC_SET_BIT_32(b, GICR_ICACTIVER, id)

#define GICR_IPRIORITYR(n)                            (0x10400 + (4 * (n)))
#define gicr_ipriorityr_set(b, id, v)                 GIC_SET_FLD_RMW_32(b, GICR_IPRIORITYR, 8, id, v)
#define gicr_ipriorityr_clr(b, id)                    GIC_CLR_FLD_RMW_32(b, GICR_IPRIORITYR, 8, id, 0xff)

#define GICR_ICFGR(n)                                 (0x10C00 + (4 * (n)))
#define gicr_icfgr_set(b, id)                         GIC_SET_FLD_RMW_32(b, GICR_ICFGR, 2, id, 0x2)
#define gicr_icfgr_clr(b, id)                         GIC_CLR_FLD_RMW_32(b, GICR_ICFGR, 2, id, 0x3)

#define GICR_IGRPMODR(n)                              (0x10D00 + (4 * (n)))
#define gicr_igrpmodr_set(b, id)                      GIC_SET_BIT_RMW_32(b, GICR_IGRPMODR, id)
#define gicr_igrpmodr_clr(b, id)                      GIC_CLR_BIT_RMW_32(b, GICR_IGRPMODR, id)

#define GICR_NSACR                                    (0x10E00)
#define gicr_nsacr_set(b, id)                         GIC_SET_BIT_RMW_32(b, GICR_NSACR, id)
#define gicr_nsacr_clr(b, id)                         GIC_CLR_BIT_RMW_32(b, GICR_NSACR, id)

#endif // _ARM_GICV3_RDIST_IF_H_
