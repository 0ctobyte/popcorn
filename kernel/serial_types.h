#ifndef _SERIAL_TYPES_H_
#define _SERIAL_TYPES_H_

#include <sys/types.h>

typedef struct {
    // Initializes the serial device
    void (*init)(void*);

    // Writes the given number of characters to the serial device
    void (*write)(void*, const char*, size_t);

    // Reads the given number of characters from the serial device
    void (*read)(void*, char*, size_t);
} serial_dev_ops_t;

typedef struct {
    void *data;            // Device specific data;
    serial_dev_ops_t *ops; // Device operations
} serial_dev_t;

extern serial_dev_t serial_dev;

#endif // _SERIAL_TYPES_H_
