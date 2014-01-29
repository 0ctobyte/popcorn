.text
.code 32

# Set to 0 if interrupts are not enabled
.comm __interrupts_enabled, 4, 4

# Enable interrupts on the processor
.global interrupts_enable
.type interrupts_enable, %function
.align 2
interrupts_enable:
	STMFD SP!, {R0-R1, LR}

	MRS R0, CPSR
	# Enable IRQ only
	BIC R0, R0, #0x80
	MSR CPSR, R0

	# Set flag to true
	LDR R0, =__interrupts_enabled
	MOV R1, #1
	STR R1, [R0]

	LDMFD SP!, {R0-R1, PC}

# Disable interrupts on the processor
.global interrupts_disable
.type interrupts_disable, %function
.align 2
interrupts_disable:
	STMFD SP!, {R0-R1, LR}
	
	MRS R0, CPSR
	# Disable IRQ and FIQ
	ORR R0, R0, #0xC0
	MSR CPSR, R0

	# Set flag to false
	LDR R0, =__interrupts_enabled
	MOV R1, #0
	STR R1, [R0]

	LDMFD SP!, {R0-R1, PC}
 
