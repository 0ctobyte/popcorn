#ifndef _ARCH_TIMER_H_
#define _ARCH_TIMER_H_

// Initializes the generic timer
void arch_timer_init(void);

// Get the clock frequency of the timer in Hz
unsigned long arch_timer_get_freq(void);

// Get the clock period of the timer in seconds
#define arch_timer_get_period() (1.0 / (double)arch_timer_get_freq())

// Get the number of ticks since the system counter was started
unsigned long arch_timer_get_ticks(void);

#define arch_timer_get_secs()  ((double)arch_timer_get_ticks() * arch_timer_get_period())
#define arch_timer_get_msecs() (arch_timer_get_secs() * 1000.0)
#define arch_timer_get_usecs() (arch_timer_get_msecs() * 1000.0)

#endif // _ARCH_TIMER_H_
