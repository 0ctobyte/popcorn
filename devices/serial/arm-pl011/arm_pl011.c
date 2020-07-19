#include "arm_pl011.h"

//#define R_UART0_BASE (0x3F201000) // Raspberry Pi 3 UART0

#define O_UART0_DR (0x0)
#define O_UART0_FR (0x18)
#define O_UART0_IMSC (0x038)

#define REG_RD32(R) (*((uint32_t*)(R)))
#define REG_WR32(R, V) *((uint32_t*)(R)) = (V)

serial_dev_ops_t arm_pl011_ops = {
    .init = arm_pl011_init,
    .write = arm_pl011_write,
    .read = arm_pl011_read
};

void arm_pl011_init(void *data) {
}

void arm_pl011_write(void *data, const char *src, size_t count) {
    arm_pl011_t *pl011 = (arm_pl011_t*)data;

    // Check if transmit FIFO is full
    for (size_t i = 0; i < count; i++) {
        while (REG_RD32(pl011->uart_base + O_UART0_FR) & (1 << 5));
        REG_WR32(pl011->uart_base + O_UART0_DR, src[i]);

        // Output a carriage return after a newline
        if (src[i] == '\n') {
            while (REG_RD32(pl011->uart_base + O_UART0_FR) & (1 << 5));
            REG_WR32(pl011->uart_base + O_UART0_DR, '\r');
        }
    }
}

void arm_pl011_read(void *data, char *dst, size_t count) {
}
