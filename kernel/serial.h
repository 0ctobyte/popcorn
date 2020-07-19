#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <kernel/kresult.h>
#include <kernel/serial_types.h>

// Kernel interface
kresult_t serial_init(void);
kresult_t serial_write(const char *src, size_t count);
kresult_t serial_read(char *dst, size_t count);

#endif // _SERIAL_H_
