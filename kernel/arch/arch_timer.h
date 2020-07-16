#ifndef _ARCH_TIMER_H_
#define _ARCH_TIMER_H_

// Initializes the generic timer
void arch_timer_init(void);

// Get the clock frequency of the counter in Hz
unsigned long arch_timer_get_freq(void);

// Get the clock period of the counter in seconds
#define arch_timer_get_period() (1.0 / (double)arch_timer_get_freq())

// Get the number of ticks since the counter was started
unsigned long arch_timer_get_ticks(void);

#define arch_timer_get_secs()  ((double)arch_timer_get_ticks() * arch_timer_get_period())
#define arch_timer_get_msecs() (arch_timer_get_secs() * 1000.0)
#define arch_timer_get_usecs() (arch_timer_get_msecs() * 1000.0)

// Enable the generic timer to fire an interrupt after the given time has passed in number of counter ticks
void arch_timer_enable_ticks(unsigned long ticks);

#define ARCH_TIMER_SECS_TO_TICKS(t)  ((double)(t) * (double)arch_timer_get_freq())
#define ARCH_TIMER_MSECS_TO_TICKS(t) (ARCH_TIMER_SECS_TO_TICKS(t) / 1000.0)
#define ARCH_TIMER_USECS_TO_TICKS(t) (ARCH_TIMER_MSECS_TO_TICKS(t) / 1000.0)

#define arch_timer_enable_secs(t)  (arch_timer_enable_ticks(ARCH_TIMER_SECS_TO_TICKS(t)))
#define arch_timer_enable_msecs(t) (arch_timer_enable_ticks(ARCH_TIMER_MSECS_TO_TICKS(t)))
#define arch_timer_enable_usecs(t) (arch_timer_enable_ticks(ARCH_TIMER_USECS_TO_TICKS(t)))

#endif // _ARCH_TIMER_H_
