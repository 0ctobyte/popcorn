/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _BCM2835_AUX_UART_IF_H_
#define _BCM2835_AUX_UART_IF_H_

#define MMIO_READ_32(base, reg)                       (*((uint32_t*)((base) + (reg))))
#define MMIO_READ_64(base, reg)                       (*((uint64_t*)((base) + (reg))))
#define MMIO_WRITE_32(base, reg, val)                 (*((uint32_t*)((base) + (reg))) = (val))
#define MMIO_WRITE_64(base, reg, val)                 (*((uint64_t*)((base) + (reg))) = (val))

#define AUX_IRQ                                       (0x0000)
#define G_AUX_IRQ_MINI_UART_IRQ(r)                    ((r) & 1)
#define aux_irq_read(b)                               (MMIO_READ_32(b, AUX_IRQ))

#define AUX_ENABLES                                   (0x0004)
#define G_AUX_ENABLES_MINI_UART_ENABLE(r)             ((r) & 1)
#define S_AUX_ENABLES_MINI_UART_ENABLE                (1)
#define aux_enables_read(b)                           (MMIO_READ_32(b, AUX_ENABLES))
#define aux_enables_write(b, r)                       (MMIO_WRITE_32(b, AUX_ENABLES, r))

#define AUX_MU_IO_REG                                 (0x0040)
#define G_AUX_MU_IO_REG_DATA(r)                       ((r) & 0xff)
#define F_AUX_MU_IO_REG_DATA(f)                       ((f) & 0xff)
#define aux_mu_io_reg_read(b)                         (MMIO_READ_32(b, AUX_MU_IO_REG))
#define aux_mu_io_reg_write(b, r)                     (MMIO_WRITE_32(b, AUX_MU_IO_REG, r))

#define AUX_MU_IER_REG                                (0x0044)
#define G_AUX_MU_IER_REG_TRANSMIT_ENABLE(r)           (((r) >> 1) & 1)
#define G_AUX_MU_IER_REG_RECEIVE_ENABLE(r)            ((r) & 1)
#define F_AUX_MU_IER_REG_TRANSMIT_ENABLE(f)           (((f) & 1) << 1)
#define F_AUX_MU_IER_REG_RECEIVE_ENABLE(f)            ((f) & 1)
#define aux_mu_ier_reg_read(b)                        (MMIO_READ_32(b, AUX_MU_IER_REG))
#define aux_mu_ier_reg_write(b, r)                    (MMIO_WRITE_32(b, AUX_MU_IER_REG, r))

#define AUX_MU_IIR_REG                                (0x0048)
#define G_AUX_MU_IIR_REG_FIFO_ENABLES(r)              (((r) >> 6) & 3)
#define G_AUX_MU_IIR_REG_INTERRUPT_ID(r)              (((r) >> 1) & 3)
#define G_AUX_MU_IIR_REG_INTERRUPT_PENDING(r)         ((r) & 1)
#define F_AUX_MU_IIR_REG_FIFO_ENABLES(r)              (((r) & 3) << 6)
#define S_AUX_MU_IIR_REG_TRANSMIT_FIFO_CLEAR          (1 << 2)
#define S_AUX_MU_IIR_REG_RECEIVE_FIFO_CLEAR           (1 << 1)
#define aux_mu_iir_reg_read(b)                        (MMIO_READ_32(b, AUX_MU_IIR_REG))
#define aux_mu_iir_reg_write(b, r)                    (MMIO_WRITE_32(b, AUX_MU_IIR_REG, r))

#define AUX_MU_LCR_REG                                (0x004C)
#define G_AUX_MU_LCR_REG_DLAB_ACCESS(r)               (((r) >> 7) & 1)
#define G_AUX_MU_LCR_REG_BREAK(r)                     (((r) >> 6) & 1)
#define G_AUX_MU_LCR_REG_DATA_SIZE(r)                 ((r) & 3)
#define S_AUX_MU_LCR_REG_DLAB_ACCESS                  (1 << 7)
#define S_AUX_MU_LCR_REG_BREAK                        (1 << 6)
#define F_AUX_MU_LCR_REG_DATA_SIZE(f)                 ((f) & 3)
#define aux_mu_lcr_reg_read(b)                        (MMIO_READ_32(b, AUX_MU_LCR_REG))
#define aux_mu_lcr_reg_write(b, r)                    (MMIO_WRITE_32(b, AUX_MU_LCR_REG, r))

#define AUX_MU_MCR_REG                                (0x0050)
#define G_AUX_MU_MCR_REG_RTS(r)                       (((r) >> 1) & 1)
#define F_AUX_MU_MCR_REG_RTS(f)                       (((f) & 1) << 1)
#define aux_mu_mcr_reg_read(b)                        (MMIO_READ_32(b, AUX_MU_MCR_REG));
#define aux_mu_mcr_reg_write(b, r)                    (MMIO_WRITE_32(b, AUX_MU_MCR_REG, r));

#define AUX_MU_LSR_REG                                (0x0054)
#define G_AUX_MU_LSR_REG_TRANSMITTER_IDLE(r)          (((r) >> 6) & 1)
#define G_AUX_MU_LSR_REG_TRANSMITTER_EMPTY(r)         (((r) >> 5) & 1)
#define G_AUX_MU_LSR_REG_RECEIVER_OVERRUN(r)          (((r) >> 1) & 1)
#define G_AUX_MU_LSR_REG_DATA_READY(r)                ((r) & 1)
#define aux_mu_lsr_reg_read(b)                        (MMIO_READ_32(b, AUX_MU_LSR_REG))

#define AUX_MU_MSR_REG                                (0x0058)
#define G_AUX_MU_MSR_REG_CTS_STATUS(r)                (((r) >> 5) & 1)
#define aux_mu_msr_reg_read(b)                        (MMIO_READ_32(b, AUX_MU_MSR_REG))

#define AUX_MU_SCRATCH                                (0x005C)
#define F_AUX_MU_SCRATCH_SCRATCH(f)                   ((f) & 0xff)
#define aux_mu_scratch_read(b)                        (MMIO_READ_32(b, AUX_MU_SCRATCH))
#define aux_mu_scratch_write(b, r)                    (MMIO_WRITE_32(b, AUX_MU_SCRATCH, r))

#define AUX_MU_CNTL_REG                               (0x0060)
#define G_AUX_MU_CNTL_REG_CTS_ASSERT_LEVEL(r)         (((r) >> 7) & 1)
#define G_AUX_MU_CNTL_REG_RTS_ASSERT_LEVEL(r)         (((r) >> 6) & 1)
#define G_AUX_MU_CNTL_REG_RTS_AUTO_FLOW_LEVEL(r)      (((r) >> 4) & 3)
#define G_AUX_MU_CNTL_REG_CTS_AUTO_FLOW_ENABLE(r)     (((r) >> 3) & 1)
#define G_AUX_MU_CNTL_REG_RTS_AUTO_FLOW_ENABLE(r)     (((r) >> 2) & 1)
#define G_AUX_MU_CNTL_REG_TRANSMITTER_ENABLE(r)       (((r) >> 1) & 1)
#define G_AUX_MU_CNTL_REG_RECEIVER_ENABLE(r)          ((r) & 1)
#define S_AUX_MU_CNTL_REG_CTS_ASSERT_LEVEL            (1 << 7)
#define S_AUX_MU_CNTL_REG_RTS_ASSERT_LEVEL            (1 << 6)
#define F_AUX_MU_CNTL_REG_RTS_AUTO_FLOW_LEVEL(f)      (((f) & 3) << 4)
#define S_AUX_MU_CNTL_REG_CTS_AUTO_FLOW_ENABLE        (1 << 3)
#define S_AUX_MU_CNTL_REG_RTS_AUTO_FLOW_ENABLE        (1 << 2)
#define S_AUX_MU_CNTL_REG_TRANSMITTER_ENABLE          (1 << 1)
#define S_AUX_MU_CNTL_REG_RECEIVER_ENABLE             (1)
#define aux_mu_cntl_reg_read(b)                       (MMIO_READ_32(b, AUX_MU_CNTL_REG))
#define aux_mu_cntl_reg_write(b, r)                   (MMIO_WRITE_32(b, AUX_MU_CNTL_REG, r))

#define AUX_MU_STAT_REG                               (0x0064)
#define G_AUX_MU_STAT_REG_TRANSMIT_FIFO_FILL_LEVEL(r) (((r) >> 24) & 0xf)
#define G_AUX_MU_STAT_REG_RECEIVE_FIFO_FILL_LEVEL(r)  (((r) >> 16) & 0xf)
#define G_AUX_MU_STAT_REG_TRANSMITTER_DONE(r)         (((r) >> 9) & 1)
#define G_AUX_MU_STAT_REG_TRANSMIT_FIFO_EMPTY(r)      (((r) >> 8) & 1)
#define G_AUX_MU_STAT_REG_CTS_LINE(r)                 (((r) >> 7) & 1)
#define G_AUX_MU_STAT_REG_RTS_STATUS(r)               (((r) >> 6) & 1)
#define G_AUX_MU_STAT_REG_TRANSMIT_FIFO_FULL(r)       (((r) >> 5) & 1)
#define G_AUX_MU_STAT_REG_RECEIVER_OVERRUN(r)         (((r) >> 4) & 1)
#define G_AUX_MU_STAT_REG_TRANSMITTER_IDLE(r)         (((r) >> 3) & 1)
#define G_AUX_MU_STAT_REG_RECEIVER_IDLE(r)            (((r) >> 2) & 1)
#define G_AUX_MU_STAT_REG_SPACE_AVAILABLE(r)          (((r) >> 1) & 1)
#define G_AUX_MU_STAT_REG_SYMBOL_AVAILABLE(r)         ((r) & 1)
#define aux_mu_stat_reg_read(b)                       (MMIO_READ_32(b, AUX_MU_STAT_REG))

#define AUX_MU_BAUD_REG                               (0x0068)
#define G_AUX_MU_BAUD_REG_BAUDRATE(r)                 ((r) & 0xffff)
#define F_AUX_MU_BAUD_REG_BAUDRATE(f)                 ((f) & 0xffff)
#define aux_mu_baud_reg_read(b)                       (MMIO_READ_32(b, AUX_MU_BAUD_REG))
#define aux_mu_baud_reg_write(b, r)                   (MMIO_WRITE_32(b, AUX_MU_BAUD_REG, r))

#endif // _BCM2835_AUX_UART_IF_H_
