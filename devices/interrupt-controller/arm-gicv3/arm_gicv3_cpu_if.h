#ifndef _ARM_GICV3_CPU_IF_H_
#define _ARM_GICV3_CPU_IF_H_

#include "arm_gicv3_common.h"

#define GICC_CTRL                                     (0x0000)
#define S_GICC_CTRL_EOIMODENS                         (1 << 9)
#define S_GICC_CTRL_IRQBYPDISGRP1                     (1 << 6)
#define S_GICC_CTRL_FIQBYPDISGRP1                     (1 << 5)
#define S_GICC_CTRL_ENABLEGRP1                        (1)
#define gicc_ctrl_read(b)                             (MMIO_READ_32(b, GICC_CTRL))
#define gicc_ctrl_write(b, r)                         (MMIO_WRITE_32(b, GICC_CTRL, r))

#define G_ICC_CTRL_EXTRANGE(r)                        (((r) >> 19) & 1)
#define G_ICC_CTRL_RSS(r)                             (((r) >> 18) & 1)
#define G_ICC_CTRL_A3V(r)                             (((r) >> 15) & 1)
#define G_ICC_CTRL_SEIS(r)                            (((r) >> 14) & 1)
#define G_ICC_CTRL_IDBITS(r)                          (((r) >> 11) & 7)
#define G_ICC_CTRL_PRIBITS(r)                         (((r) >> 8) & 7)
#define G_ICC_CTRL_PMHE(r)                            (((r) >> 6) & 1)
#define G_ICC_CTRL_EOIMODE(r)                         (((r) >> 1) & 1)
#define G_ICC_CTRL_CBPR(r)                            ((r) & 1)
#define S_ICC_CTRL_PMHE                               (1 << 6)
#define S_ICC_CTRL_EOIMODE                            (1 << 1)
#define S_ICC_CTRL_CBPR                               (1)
static inline uint32_t icc_ctrl_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_4" : "=r" (v));
    return v;
}
static inline void icc_ctrl_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C12_4, %0" :: "r" (v));
}

#define GICC_PMR                                      (0x0004)
#define G_GICC_PMR_PRIORITY(r)                        ((r) & 0xff)
#define F_GICC_PMR_PRIORITY(f)                        ((f) & 0xff)
#define gicc_pmr_read(b)                              (MMIO_READ_32(b, GICC_PMR))
#define gicc_pmr_write(b, r)                          (MMIO_WRITE_32(b, GICC_PMR, r))
static inline uint32_t icc_pmr_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C4_C6_0" : "=r" (v));
    return v;
}
static inline void icc_pmr_el1_w(uint32_t v) {
    asm ("msr S3_0_C4_C6_0, %0" :: "r" (v));
}

#define GICC_BPR                                      (0x0008)
#define G_GICC_BPR_PRIORITY(r)                        ((r) & 0x7)
#define F_GICC_BPR_PRIORITY(f)                        ((f) & 0x7)
#define gicc_bpr_read(b)                              (MMIO_READ_32(b, GICC_BPR))
#define gicc_bpr_write(b, r)                          (MMIO_WRITE_32(b, GICC_BPR, r))
static inline uint32_t icc_bpr0_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_3" : "=r" (v));
    return v;
}
static inline void icc_bpr0_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C8_3, %0" :: "r" (v));
}
static inline uint32_t icc_bpr1_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_3" : "=r" (v));
    return v;
}
static inline void icc_bpr1_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C12_3, %0" :: "r" (v));
}

#define GICC_IAR                                      (0x000C)
#define G_GICC_IAR_INTID(r)                           ((r) & 0xffffff)
#define gicc_iar_read(b)                              (MMIO_READ_32(b, GICC_IAR))
static inline uint32_t icc_iar0_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_0" : "=r" (v));
    return v;
}
static inline uint32_t icc_iar1_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_0" : "=r" (v));
    return v;
}

#define GICC_EOIR                                     (0x0010)
#define F_GICC_EOIR_INTID(r)                          ((r) & 0xffffff)
#define gicc_eoir_write(b, r)                         (MMIO_WRITE_32(b, GICC_EOIR, r))
static inline void icc_eoir0_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C8_1, %0" :: "r" (v));
}
static inline void icc_eoir1_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C12_1, %0" :: "r" (v));
}

#define GICC_RPR                                      (0x0014)
#define G_GICC_RPR_PRIORITY(r)                        ((r) & 0xff)
#define gicc_rpr_read(b)                              (MMIO_READ_32(b, GICC_RPR))
static inline uint32_t icc_rpr_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C11_3" : "=r" (v));
    return v;
}

#define GICC_HPPIR                                    (0x0018)
#define G_GICC_HPPIR_INTID(r)                         ((r) & 0xffffff)
#define gicc_hppir_read(b)                            (MMIO_READ_32(b, GICC_HPPIR))
static inline uint32_t icc_hppir0_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_2" : "=r" (v));
    return v;
}
static inline uint32_t icc_hppir1_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_2" : "=r" (v));
    return v;
}

#define GICC_ABPR                                     (0x001C)
#define G_GICC_ABPR_PRIORITY(r)                       ((r) & 0x7)
#define F_GICC_ABPR_PRIORITY(f)                       ((f) & 0x7)
#define gicc_abpr_read(b)                             (MMIO_READ_32(b, GICC_ABPR))
#define gicc_abpr_write(b, r)                         (MMIO_WRITE_32(b, GICC_ABPR, r))

#define GICC_AIAR                                     (0x0020)
#define G_GICC_AIAR_INTID(r)                          ((r) & 0xffffff)
#define gicc_aiar_read(b)                             (MMIO_READ_32(b, GICC_AIAR))

#define GICC_AEOIR                                    (0x0024)
#define F_GICC_AEOIR_INTID(r)                         ((r) & 0xffffff)
#define gicc_aeoir_write(b, r)                        (MMIO_WRITE_32(b, GICC_AEOIR, r))

#define GICC_AHPPIR                                   (0x0028)
#define G_GICC_AHPPIR_INTID(r)                        ((r) & 0xffffff)
#define gicc_ahppir_read(b)                           (MMIO_READ_32(b, GICC_AHPPIR))

#define GICC_STATUSR                                  (0x002C)
#define G_GICC_STATUSR_ASV(r)                         (((r) >> 4) & 1)
#define G_GICC_STATUSR_WROD(r)                        (((r) >> 3) & 1)
#define G_GICC_STATUSR_RWOD(r)                        (((r) >> 2) & 1)
#define G_GICC_STATUSR_WRD(r)                         (((r) >> 1) & 1)
#define G_GICC_STATUSR_RRD(r)                         ((r) & 1)
#define gicc_statusr_read(b)                          (MMIO_READ_32(b, GICC_STATUSR))

#define GICC_APR(n)                                   (0x00D0 + (4 * (n)))
#define gicc_apr_read(b, id)                          (MMIO_READ_32(b, GICC_APR(id / 32))
static inline uint32_t icc_ap0r0_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_4" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap0r1_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_5" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap0r2_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_6" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap0r3_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C8_7" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap1r0_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C9_0" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap1r1_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C9_1" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap1r2_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C9_2" : "=r" (v));
    return v;
}
static inline uint32_t icc_ap1r3_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C9_3" : "=r" (v));
    return v;
}

#define GICC_NSAPR(n)                                 (0x00E0 + (4 * (n)))
#define gicc_apr_read(b, id)                          (MMIO_READ_32(b, GICC_APR(id / 32))

#define GICC_IIDR                                     (0x00FC)
#define G_GICC_IIDR_PRODUCTID(r)                      (((r) >> 20) & 0xfff)
#define G_GICC_IIDR_ARCHITECTURE_VERSION(r)           (((r) >> 16) & 0xf)
#define G_GICC_IIDR_REVISION(r)                       (((r) >> 12) & 0xf)
#define G_GICC_IIDR_IMPLEMENTER(r)                    ((r) & 0xfff)
#define gicc_iidr_read(b)                             (MMIO_READ_32(b, GICC_IIDR)

#define GICC_DIR                                      (0x1000)
#define F_GICC_DIR_INTID(r)                           ((r) & 0xffffff)
#define gicc_dir_write(b, r)                          (MMIO_WRITE_32(b, GICC_DIR, r))
static inline void icc_dir_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C11_1, %0" :: "r" (v));
}

static inline void icc_sgi0r_el1_w(uint64_t v) {
    asm ("msr S3_0_C12_C11_7, %0" :: "r" (v));
}
static inline void icc_sgi1r_el1_w(uint64_t v) {
    asm ("msr S3_0_C12_C11_5, %0" :: "r" (v));
}
static inline void icc_asgi1r_el1_w(uint64_t v) {
    asm ("msr S3_0_C12_C11_6, %0" :: "r" (v));
}

static inline uint32_t icc_sre_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_5" : "=r" (v));
    return v;
}
static inline void icc_sre_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C12_5, %0" :: "r" (v));
}

static inline uint32_t icc_igrpen0_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_6" : "=r" (v));
    return v;
}
static inline void icc_igrpen0_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C12_6, %0" :: "r" (v));
}
static inline uint32_t icc_igrpen1_el1_r(void) {
    uint32_t v;
    asm ("mrs %0, S3_0_C12_C12_7" : "=r" (v));
    return v;
}
static inline void icc_igrpen1_el1_w(uint32_t v) {
    asm ("msr S3_0_C12_C12_7, %0" :: "r" (v));
}

#endif // _ARM_GICV3_CPU_IF_H_
