.text

# Get generic timer frequency
.global arch_timer_get_freq
.align 2
arch_timer_get_freq:
    mrs x0, CNTFRQ_EL0
    ret lr

# Get the current ticks since the system counter was started
.global arch_timer_get_ticks
.align 2
arch_timer_get_ticks:
    mrs x0, CNTVCT_EL0
    ret lr

# Enables the timer to fire after the given number of ticks
.global arch_timer_enable_ticks
.align 2
arch_timer_enable_ticks:
    msr CNTV_TVAL_EL0, x0
    mov x0, #1
    msr CNTV_CTL_EL0, x0
    isb sy
    ret lr
