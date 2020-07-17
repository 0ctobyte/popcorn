.text

# Enable interrupts on the processor
.global arch_interrupts_enable
.align 2
arch_interrupts_enable:
    msr DAIFclr, #0x3
    ret lr

# Disable interrupts on the processor
.global arch_interrupts_disable
.align 2
arch_interrupts_disable:
    msr DAIFset, #0x3
    ret lr

# True if interrupts enabled, false otherwise
.global arch_interrupts_is_enabled
.align 2
arch_interrupts_is_enabled:
    # Check if IRQ's are enabled
    mrs x0, DAIF
    tst x0, #0x80
    cset x0, eq
    ret lr
