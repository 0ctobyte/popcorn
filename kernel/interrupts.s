.text
.code 32

.global enable_interrupts
.type enable_interrupts, %function

.global disable_interrupts
.type disable_interrupts, %function

.global interrupts_enabled
.type interrupts_enabled, %function

.global register_isr
.type register_isr, %function

.global irq_init
.type irq_init, %function

.global enable_irq
.type enable_irq, %function

.global disable_irq
.type disable_irq, %function

.global sic_isr
.type sic_isr, %function

.global irq_handler
.global swi_handler
.global undef_handler
.global prefetch_abort_handler
.global data_abort_handler
.global fiq_handler

/* 
 * This symbol will point to the ISR table with a total of 64 entries
 */
isr_table:
	.skip 64*4, 0

/*
 * Enable interrupts on the processor
 * Both IRQ and FIQ interrupts are enabled
 */
enable_interrupts:
	STMFD SP!, {R0, LR}

	MRS R0, CPSR
	BIC R0, R0, #0xC0 /* Enable IRQ and FIQ */
	MSR CPSR, R0	

	LDMFD SP!, {R0, PC}

/*
 * Disable interrupts on the processor
 */
disable_interrupts:
	STMFD SP!, {R0, LR}
	
	MRS R0, CPSR
	ORR R0, R0, #0xC0 /* Disable IRQ and FIQ */
	MSR CPSR, R0

	LDMFD SP!, {R0, PC}
/*
 * Check if interrupts are enabled
 * Returns 1 if either FIQ or IRQ or both are enabled
 * Returns 0 if both FIQ and IRQ are disabled
 */
interrupts_enabled:
	STMFD SP!, {R1, LR}

	MRS R1, CPSR
	ANDS R1, #0x000000C0
	BEQ A1
	MOV R0, #0
	LDMFD SP!, {R1, PC}
A1:
	MOV R0, #1
	LDMFD SP!, {R1, PC}

/*
 * Register a ISR in the isr table
 */
register_isr:
	STMFD SP!, {R0, R4, LR}
	
	LDR R4, =isr_table
	LSL R0, R0, #2 /* Multiply by 4, since 4 bytes per index */
	STR R1, [R4, R0] 

	LDMFD SP!, {R0, R4, PC}

/*
 * Initializes the IRQ systems, including the VIC and SIC
 */
irq_init:
	STMFD SP!, {R0, R1, LR}

	BL vic_init
	BL sic_init

	/* Enable IRQ_SIC */
	MOV R0, #31
	BL enable_irq

	/* Register ISR for IRQ_SIC */
	LDR R1, =sic_isr
	BL register_isr
	
	BL enable_interrupts

	LDMFD SP!, {R0, R1, PC}

/*
 * Enables IRQ for the specified source on the VIC and SIC
 */
enable_irq:
	STMFD SP!, {R0, R1, R2, LR}

	MOV R2, #1
	SUBS R1, R0, #32

	/* If IRQ # is less than 32, than simply set bit in VICENABLE register */ 
	BMI B0

	/* Otherwise, source is connected to SIC */
	/* Set the bit in the SICENABLE register */
	/* Assuming bit 31 (SIC IRQ) on the VICENABLE register is already set */
	LSL R0, R2, R1
	BL sic_enable	
	LDMFD SP!, {R0, R1, R2, PC}
B0:
	LSL R0, R2, R0
	BL vic_enable_irq		
	LDMFD SP!, {R0, R1, R2, PC}

/*
 * Disables IRQ for the specified source on the VIC and SIC
 */
disable_irq:
	STMFD SP!, {R0, R1, R2, LR}

	MOV R2, #1
	SUBS R1, R0, #32

	/* If IRQ# is less than 32, simple disable in VIC */
	BMI C0

	/* Otherwise, source is connected to SIC */
	LSL R0, R2, R1
	BL sic_disable
	LDMFD SP!, {R0, R1, R2, PC}
C0:
	LSL R0, R2, R0
	BL vic_disable_irq
	LDMFD SP!, {R0, R1, R2, PC}

/*
 * This function is the ISR for the SIC
 */
sic_isr:
	STMFD SP!, {R0-R6, LR}

	MOV R1, #0 /* IRQ# on SIC */
	MOV R2, #32 /* IRQ# in isr_table */
	MOV R3, #1 /* Bit shifter operand */
	LDR R4, =isr_table /* Pointer to isr_table */

	/* Check the SIC status register for raised interrupts */
	BL sic_status
D0:
	CMP R1, #32
	BEQ D2
	ANDS R5, R0, R3, LSL R1
	BNE D1
	ADD R1, R1, #1
	ADD R2, R2, #1
	B D0
D1:
	LSL R5, R2, #2
	LDR R6, [R4, R5]
	TEQ R6, #0 /* Check if address is valid, i.e. not NULL */
	BEQ D0
	BLX R6
	ADD R1, R1, #1
	ADD R2, R2, #1
	B D0
D2:
	LDMFD SP!, {R0-R6, PC}

/*
 * This function is called whenever an IRQ arrives from the VIC
 */
irq_handler:
	STMFD SP!, {R0-R5, LR}

	MOV R1, #0 /* IRQ# */
	MOV R2, #1 /* Bit shifter operand */
	LDR R3, =isr_table /* Pointer to isr_table */
	
	/* Check the VIC status register for raised interrupts */
	/* In this scheme, devices with lower IRQ #'s get higher priority */
	BL vic_status_irq
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
	TEQ R5, #0 /* Check if address is valid, i.e. not NULL */
	BEQ E0
	BLX R5
	ADD R1, R1, #1
	B E0
E2:
	LDMFD SP!, {R0-R5, LR}
	SUBS PC, LR, #4	

swi_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

undef_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

prefetch_abort_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

data_abort_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

fiq_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

