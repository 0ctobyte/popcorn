#include <kernel/serial_types.h>
#include <kernel/serial.h>

serial_dev_t serial_dev;

kresult_t serial_init(void) {
    if (serial_dev.ops == NULL || serial_dev.ops->init == NULL) {
        return KRESULT_UNIMPLEMENTED;
    }

    serial_dev.ops->init(serial_dev.data);

    return KRESULT_OK;
}

kresult_t serial_write(const char *src, size_t count) {
    if (serial_dev.ops == NULL || serial_dev.ops->write == NULL) {
        return KRESULT_UNIMPLEMENTED;
    }

    serial_dev.ops->write(serial_dev.data, src, count);

    return KRESULT_OK;
}

kresult_t serial_read(char *dst, size_t count) {
    if (serial_dev.ops == NULL || serial_dev.ops->read == NULL) {
        return KRESULT_UNIMPLEMENTED;
    }

    serial_dev.ops->read(serial_dev.data, dst, count);

    return KRESULT_OK;
}
