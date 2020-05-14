.text

# Enable interrupts on the processor
.global interrupts_enable
.align 2
interrupts_enable:
    msr DAIFclr, #0x3
    ret lr

# Disable interrupts on the processor
.global interrupts_disable
.align 2
interrupts_disable:
    msr DAIFset, #0x3
    ret lr

# True if interrupts enabled, false otherwise
.global interrupts_enabled
.align 2
interrupts_enabled:
    # Check if IRQ's are enabled
    mrs x0, DAIF
    tst x0, #0x80
    cset x0, eq
    ret lr
