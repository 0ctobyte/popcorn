#include "bcm2387_miniuart.h"

#define O_MU_IO   (0x00)
#define O_MU_LSR  (0x14)
#define O_MU_STAT (0x24)
#define O_MU_IIR  (0x08)

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

serial_dev_ops_t bcm2387_miniuart_ops = {
    .init = bcm2387_miniuart_init,
    .write = bcm2387_miniuart_write,
    .read = bcm2387_miniuart_read
};

void bcm2387_miniuart_init(void *data) {
}

void bcm2387_miniuart_write(void *data, const char *src, size_t count) {
    bcm2387_miniuart_t *miniuart = (bcm2387_miniuart_t*)data;

    // Check if transmit FIFO is full
    for (size_t i = 0; i < count; i++) {
        while (!(REG_RD32(miniuart->uart_base + O_MU_LSR) & (1 << 5)));
        REG_WR32(miniuart->uart_base + O_MU_IO, src[i]);

        // Output a carriage return after a newline
        if (src[i] == '\n') {
            while (!(REG_RD32(miniuart->uart_base + O_MU_LSR) & (1 << 5)));
            REG_WR32(miniuart->uart_base + O_MU_IO, '\r');
        }
    }
}

void bcm2387_miniuart_read(void *data, char *dst, size_t count) {
}
