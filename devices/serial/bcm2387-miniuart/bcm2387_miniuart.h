#ifndef _BCM2387_MINIUART_H_
#define _BCM2387_MINIUART_H_

#include <sys/types.h>
#include <kernel/console_types.h>

typedef struct {
    uintptr_t uart_base;
} bcm2387_miniuart_t;

extern console_dev_ops_t bcm2387_miniuart_ops;

void bcm2387_miniuart_init(void *data);
int bcm2387_miniuart_write(void *data, const char *src, size_t count);
int bcm2387_miniuart_read(void *data, char *dst, size_t count);

#endif // _BCM2387_MINIUART_H_
