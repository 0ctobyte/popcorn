/*
 * Copyright (c) 2020 Sekhar Bhattacharya
 *
 * SPDS-License-Identifier: MIT
 */

#ifndef _BCM2835_AUX_UART_H_
#define _BCM2835_AUX_UART_H_

#include <sys/types.h>
#include <kernel/console_types.h>

typedef enum {
    BCM2835_AUX_UART_CBITS_7 = 0,
    BCM2835_AUX_UART_CBITS_8 = 3,
} bcm2835_aux_uart_cbits_t;

typedef enum {
    BCM2835_AUX_UART_BAUD_460800 = 460800,
    BCM2835_AUX_UART_BAUD_230400 = 230400,
    BCM2835_AUX_UART_BAUD_115200 = 115200,
    BCM2835_AUX_UART_BAUD_76800 = 76800,
    BCM2835_AUX_UART_BAUD_57600 = 57600,
    BCM2835_AUX_UART_BAUD_38400 = 38400,
    BCM2835_AUX_UART_BAUD_19200 = 19200,
    BCM2835_AUX_UART_BAUD_14400 = 14400,
    BCM2835_AUX_UART_BAUD_9600 = 9600,
    BCM2835_AUX_UART_BAUD_2400 = 2400,
    BCM2835_AUX_UART_BAUD_1200 = 1200,
    BCM2835_AUX_UART_BAUD_110 = 110,
} bcm2835_aux_uart_baud_t;

typedef struct {
    uintptr_t uart_base;
    unsigned int uart_clock;        // System clock in Hz
    bcm2835_aux_uart_baud_t baud;   // Baud rate
    bcm2835_aux_uart_cbits_t cbits; // Number of bits per character
} bcm2835_aux_uart_t;

extern console_dev_ops_t bcm2835_aux_uart_ops;

void bcm2835_aux_uart_init(void *data);
int bcm2835_aux_uart_write(void *data, const char *src, size_t count);
int bcm2835_aux_uart_read(void *data, char *dst, size_t count);

#endif // _BCM2835_AUX_UART_H_
