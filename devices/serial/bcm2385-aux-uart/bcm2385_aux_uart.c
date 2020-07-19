#include "bcm2385_aux_uart.h"

#define O_MU_IO   (0x00)
#define O_MU_LSR  (0x14)
#define O_MU_STAT (0x24)
#define O_MU_IIR  (0x08)

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

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
        if ((REG_RD32(miniuart->uart_base + O_MU_LSR) & (1 << 5)) == 0) {
            return i;
        }

        REG_WR32(miniuart->uart_base + O_MU_IO, src[i]);

        // Output a carriage return after a newline
        if (src[i] == '\n') {
            while ((REG_RD32(miniuart->uart_base + O_MU_LSR) & (1 << 5)) == 0);
            REG_WR32(miniuart->uart_base + O_MU_IO, '\r');
        }
    }
}

int bcm2385_aux_uart_read(void *data, char *dst, size_t count) {
}
