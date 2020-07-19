#ifndef _BCM2385_AUX_UART_H_
#define _BCM2385_AUX_UART_H_

#include <sys/types.h>
#include <kernel/console_types.h>

typedef struct {
    uintptr_t uart_base;
} bcm2385_aux_uart_t;

extern console_dev_ops_t bcm2385_aux_uart_ops;

void bcm2385_aux_uart_init(void *data);
int bcm2385_aux_uart_write(void *data, const char *src, size_t count);
int bcm2385_aux_uart_read(void *data, char *dst, size_t count);

#endif // _BCM2385_AUX_UART_H_
