#include <kernel/console_types.h>
#include <kernel/console.h>

console_dev_t console_dev;

kresult_t console_init(void) {
    if (console_dev.ops == NULL || console_dev.ops->init == NULL) {
        return KRESULT_UNIMPLEMENTED;
    }

    console_dev.ops->init(console_dev.data);

    return KRESULT_OK;
}

kresult_t console_write(const char *src, size_t *count) {
    if (console_dev.ops == NULL || console_dev.ops->write == NULL) {
        return KRESULT_UNIMPLEMENTED;
    }

    *count = console_dev.ops->write(console_dev.data, src, *count);

    return KRESULT_OK;
}

kresult_t console_read(char *dst, size_t *count) {
    if (console_dev.ops == NULL || console_dev.ops->read == NULL) {
        return KRESULT_UNIMPLEMENTED;
    }

    *count = console_dev.ops->read(console_dev.data, dst, *count);

    return KRESULT_OK;
}
