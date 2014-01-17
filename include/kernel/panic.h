#ifndef __PANIC_H__
#define __PANIC_H__

#define HALT() for(;;)

void panic(const char *fmt, ...);

#endif // __PANIC_H__

