#ifndef _ARM_PL011_H_
#define _ARM_PL011_H_

#include <sys/types.h>
#include <kernel/serial_types.h>

typedef struct {
    uintptr_t uart_base;
} arm_pl011_t;

extern serial_dev_ops_t arm_pl011_ops;

void arm_pl011_init(void *data);
void arm_pl011_write(void *data, const char *src, size_t count);
void arm_pl011_read(void *data, char *dst, size_t count);

#endif // _ARM_PL011_H_
