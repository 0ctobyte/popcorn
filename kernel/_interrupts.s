.text
.code 32

# Enable interrupts on the processor
.global interrupts_enable
.type interrupts_enable, %function
.align 2
interrupts_enable:
	mrs r0, cpsr
	# Enable IRQ and FIQ
	bic r0, r0, #0xc0
	msr cpsr, r0

  bx lr

# Disable interrupts on the processor
.global interrupts_disable
.type interrupts_disable, %function
.align 2
interrupts_disable:
	mrs r0, cpsr
	# Disable IRQ and FIQ
	orr r0, r0, #0xc0
	msr cpsr, r0

  bx lr

# True if interrupts enabled, false otherwise
.global interrupts_enabled
.type interrupts_enabled, %function
.align 2
interrupts_enabled:
	mrs r0, cpsr
	# Check if IRQ's are enabled
	tst r0, #0x80
	moveq r0, #1
	movne r0, #0

  bx lr
 
