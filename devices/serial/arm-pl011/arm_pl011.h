/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _ARM_PL011_H_
#define _ARM_PL011_H_

#include <sys/types.h>
#include <kernel/console_types.h>

typedef enum {
    ARM_PL011_CBITS_5 = 0,
    ARM_PL011_CBITS_6 = 1,
    ARM_PL011_CBITS_7 = 2,
    ARM_PL011_CBITS_8 = 3,
} arm_pl011_cbits_t;

typedef enum {
    ARM_PL011_BAUD_460800 = 460800,
    ARM_PL011_BAUD_230400 = 230400,
    ARM_PL011_BAUD_115200 = 115200,
    ARM_PL011_BAUD_76800 = 76800,
    ARM_PL011_BAUD_57600 = 57600,
    ARM_PL011_BAUD_38400 = 38400,
    ARM_PL011_BAUD_19200 = 19200,
    ARM_PL011_BAUD_14400 = 14400,
    ARM_PL011_BAUD_9600 = 9600,
    ARM_PL011_BAUD_2400 = 2400,
    ARM_PL011_BAUD_1200 = 1200,
    ARM_PL011_BAUD_110 = 110,
} arm_pl011_baud_t;

typedef struct {
    uintptr_t uart_base;
    double uart_clock;       // UART clock frequency in Hz
    arm_pl011_baud_t baud;   // Baud rate
    arm_pl011_cbits_t cbits; // Number of bits per character
    bool two_stop_bits;      // Number of stop bits
    bool even_parity;        // Even parity mode;
    bool enable_parity;      // Enable parity bit and checking
} arm_pl011_t;

extern console_dev_ops_t arm_pl011_ops;

void arm_pl011_init(void *data);
int arm_pl011_write(void *data, const char *src, size_t count);
int arm_pl011_read(void *data, char *dst, size_t count);

#endif // _ARM_PL011_H_
