#ifndef _PLATFORM_H_
#define _PLATFORM_H_

// This is invoked before the virtual memory system is setup
void platform_early_init(void);

// This is invoked for further initialization after the virtual memory system is setup
void platform_init(void);

#endif // _PLATFORM_H_
