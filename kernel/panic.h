#ifndef _PANIC_H_
#define _PANIC_H_

#define HALT() for(;;)

void panic(const char *fmt, ...);

#endif // _PANIC_H_
