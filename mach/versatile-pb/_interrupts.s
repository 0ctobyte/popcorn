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
 
# This function is called whenever an IRQ arrives from the VIC
.global irq_handler
.type irq_handler, %function
.align 2
irq_handler:
	STMFD SP!, {R0-R5, LR}

	MOV R1, #0 /* IRQ# */
	MOV R2, #1 /* Bit shifter operand */
	LDR R3, =isr_table /* Pointer to isr_table */
	
	/* Check the VIC status register for raised interrupts */
	/* In this scheme, devices with lower IRQ #'s get higher priority */
	BL vic_status
E0:
	CMP R1, #32
	BEQ E2
	ANDS R4, R0, R2, LSL R1
	BNE E1
	ADD R1, R1, #1
	B E0
E1:
	LSL R4, R1, #2
	LDR R5, [R3, R4]
	ADD R1, R1, #1
	TEQ R5, #0 /* Check if address is valid, i.e. not NULL */
	BEQ E0
	BLX R5
	B E0
E2:
	LDMFD SP!, {R0-R5, LR}
	SUBS PC, LR, #4	

