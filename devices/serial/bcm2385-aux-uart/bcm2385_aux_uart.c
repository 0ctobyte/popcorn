#include "bcm2385_aux_uart_if.h"
#include "bcm2385_aux_uart.h"

console_dev_ops_t bcm2385_aux_uart_ops = {
    .init = bcm2385_aux_uart_init,
    .write = bcm2385_aux_uart_write,
    .read = bcm2385_aux_uart_read
};

void bcm2385_aux_uart_init(void *data) {
}

int bcm2385_aux_uart_write(void *data, const char *src, size_t count) {
    bcm2385_aux_uart_t *miniuart = (bcm2385_aux_uart_t*)data;

    for (size_t i = 0; i < count; i++) {
        // Check if transmit FIFO is full
        if (G_AUX_MU_STAT_REG_SPACE_AVAILABLE(aux_mu_stat_reg_read(miniuart->uart_base)) == 0) {
            return i;
        }

        aux_mu_io_reg_write(miniuart->uart_base, F_AUX_MU_IO_REG_DATA(src[i]));

        // Output a carriage return after a newline
        if (src[i] == '\n') {
            while (G_AUX_MU_STAT_REG_SPACE_AVAILABLE(aux_mu_stat_reg_read(miniuart->uart_base)) == 0);
            aux_mu_io_reg_write(miniuart->uart_base, F_AUX_MU_IO_REG_DATA('\r'));
        }
    }

    return count;
}

int bcm2385_aux_uart_read(void *data, char *dst, size_t count) {
    bcm2385_aux_uart_t *miniuart = (bcm2385_aux_uart_t*)data;

    for (size_t i = 0; i < count; i++) {
        // Check if receive FIFO is empty
        if (G_AUX_MU_STAT_REG_SYMBOL_AVAILABLE(aux_mu_stat_reg_read(miniuart->uart_base)) == 0) {
            return i;
        }

        dst[i] = G_AUX_MU_IO_REG_DATA(aux_mu_io_reg_read(miniuart->uart_base));
    }

    return count;
}
