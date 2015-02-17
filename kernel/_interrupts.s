.text
.code 32

# Enable interrupts on the processor
.global interrupts_enable
.type interrupts_enable, %function
.align 2
interrupts_enable:
	MRS R0, CPSR
	# Enable IRQ and FIQ
	BIC R0, R0, #0xC0
	MSR CPSR, R0

  BX LR

# Disable interrupts on the processor
.global interrupts_disable
.type interrupts_disable, %function
.align 2
interrupts_disable:
	MRS R0, CPSR
	# Disable IRQ and FIQ
	ORR R0, R0, #0xC0
	MSR CPSR, R0

  BX LR

# True if interrupts enabled, false otherwise
.global interrupts_enabled
.type interrupts_enabled, %function
.align 2
interrupts_enabled:
	MRS R0, CPSR
	# Check if IRQ's are enabled
	ANDS R0, R0, #0x80
	MOVEQ R0, #1
	MOVNE R0, #0

  BX LR
 
