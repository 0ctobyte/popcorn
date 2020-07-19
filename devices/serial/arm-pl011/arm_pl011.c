#include "arm_pl011_if.h"
#include "arm_pl011.h"

serial_dev_ops_t arm_pl011_ops = {
    .init = arm_pl011_init,
    .write = arm_pl011_write,
    .read = arm_pl011_read
};

void arm_pl011_init(void *data) {
    arm_pl011_t *pl011 = (arm_pl011_t*)data;

    // Disable the UART
    uartcr_write(pl011->uart_base, 0);

    // Disable interrupts and clear interrupt status
    uartimsc_write(pl011->uart_base, 0x3ff);
    uarticr_write(pl011->uart_base, 0x3ff);
    uartdmacr_write(pl011->uart_base, 0x0);

    // Setup the baud rate divisor: BRD = UARTCLOCK / (16 * BAUDRATE)
    // BRDi = int(BRD)
    // BRDf = int((BRD - BRDi) * 64 + 0.5)
    double brd = pl011->uart_clock / (16 * pl011->baud);
    unsigned int brdi = (unsigned int)brd;
    unsigned int brdf = (unsigned int)((brd - (double)brdi) * 64 + 0.5);

    uartibrd_write(pl011->uart_base, F_UARTIBRD_BAUD_DIVINT(brdi));
    uartfbrd_write(pl011->uart_base, F_UARTFBRD_BAUD_DIVFRAC(brdf));

    // Set up the line control register
    uint32_t lcr = F_UARTLCR_H_WLEN(pl011->cbits) | S_UARTLCR_H_FEN | (pl011->two_stop_bits ? S_UARTLCR_H_STP2 : 0)
        | (pl011->even_parity ? S_UARTLCR_H_EPS : 0) | (pl011->enable_parity ? S_UARTLCR_H_PEN : 0);
    uartlcr_h_write(pl011->uart_base, lcr);

    // Finally enable the UART
    uartcr_write(pl011->uart_base, S_UARTCR_UARTEN | S_UARTCR_RXE | S_UARTCR_TXE);
}

int arm_pl011_write(void *data, const char *src, size_t count) {
    arm_pl011_t *pl011 = (arm_pl011_t*)data;

    for (size_t i = 0; i < count; i++) {
        // Check if transmit FIFO is full
        if (G_UARTFR_TXFF(uartfr_read(pl011->uart_base)) != 0) {
            return i;
        }

        uartdr_write(pl011->uart_base, F_UARTDR_DATA(src[i]));

        // Output a carriage return after a newline
        if (src[i] == '\n') {
            while (G_UARTFR_TXFF(uartfr_read(pl011->uart_base)) != 0);
            uartdr_write(pl011->uart_base, F_UARTDR_DATA('\r'));
        }
    }

    return count;
}

int arm_pl011_read(void *data, char *dst, size_t count) {
    arm_pl011_t *pl011 = (arm_pl011_t*)data;

    for (size_t i = 0; i < count; i++) {
        // Check if receive FIFO is empty
        if (G_UARTFR_RXFE(uartfr_read(pl011->uart_base)) != 0) {
            return i;
        }

        uint32_t data = uartdr_read(pl011->uart_base);

        // Check for receive errors
        if (G_UARTDR_ANYERR(data) != 0) {
            return i;
        }

        dst[i] = G_UARTDR_DATA(data);
    }

    return count;
}
