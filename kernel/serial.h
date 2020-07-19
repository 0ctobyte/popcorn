#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <kernel/kresult.h>
#include <kernel/serial_types.h>

/* serial - Common generic serial interface for a variety of serial devices
 * This is mostly needed for early kernel initialization and for printing errors when no display drivers are available.
 * A serial device may or may not actually be connected/setup by platform code. In that case the functions here won't
 * do anything.
 */

// Kernel interface
kresult_t serial_init(void);

// The number of characters that need to read or written is provided in count
// It's also used to return the number of characters that were actually read/written
kresult_t serial_write(const char *src, size_t *count);
kresult_t serial_read(char *dst, size_t *count);

#endif // _SERIAL_H_
