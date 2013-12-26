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

.global irq_handler
.global swi_handler
.global undef_handler
.global prefetch_abort_handler
.global data_abort_handler
.global fiq_handler

/* 
 * This symbol will hold the register table with a total of 64 entries
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

	BEQ c1

c0:
	MOV R0, #0
	LDMFD SP!, {R1, PC}
c1:
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

enable_irq:
	STMFD SP!, {LR}

	

	LDMFD SP!, {PC}

irq_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
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

