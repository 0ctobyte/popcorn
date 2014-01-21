.text
.code 32

/* Addresses for the exception handlers */
.align 2
reset_handler_addr: .word _start
undef_handler_addr: .word undef_handler
swi_handler_addr: .word swi_handler
prefetch_abort_handler_addr: .word prefetch_abort_handler
data_abort_handler_addr: .word data_abort_handler
irq_handler_addr: .word irq_handler
fiq_handler_addr: .word fiq_handler

/* 
 * This symbol will point to the ISR table with a total of 64 entries
 */
.comm isr_table, 64*4, 4

/* The vector base register requires that the base address is 32 bit aligned */
.align 5
_vectors_start:
	LDR PC, reset_handler_addr
	LDR PC, undef_handler_addr
	LDR PC, swi_handler_addr
	LDR PC, prefetch_abort_handler_addr
	LDR PC, data_abort_handler_addr
	B . /* Reserved */
	LDR PC, irq_handler_addr
	LDR PC, fiq_handler_addr

.global vectors_install
.type vectors_install, %function
.align 2
vectors_install:
	STMFD SP!, {R0, R1, LR}
	
	/* Setup the vector base address register */
	LDR R0, =_vectors_start
	MRC P15, 0, R1, C12, C0, 0
	ORR R1, R1, R0
	MCR P15, 0, R1, C12, C0, 0

	LDMFD SP!, {R0, R1, PC}

/*
 * Enable interrupts on the processor
 * Both IRQ and FIQ interrupts are enabled
 */
.global enable_interrupts
.type enable_interrupts, %function
.align 2
enable_interrupts:
	STMFD SP!, {R0, LR}

	MRS R0, CPSR
	BIC R0, R0, #0xC0 /* Enable IRQ and FIQ */
	MSR CPSR, R0	

	LDMFD SP!, {R0, PC}

/*
 * Disable interrupts on the processor
 */
.global disable_interrupts
.type disable_interrupts, %function
.align 2
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
.global interrupts_enabled
.type interrupts_enabled, %function
.align 2
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
.global register_isr
.type register_isr, %function
.align 2
register_isr:
	STMFD SP!, {R0, R4, LR}
	
	LDR R4, =isr_table
	LSL R0, R0, #2 /* Multiply by 4, since 4 bytes per index */
	STR R1, [R4, R0] 

	LDMFD SP!, {R0, R4, PC}

/*
 * Initializes the IRQ systems, including the VIC and SIC
 */
.global irq_init
.type irq_init, %function
.align 2
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
.global enable_irq
.type enable_irq, %function
.align 2
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
	BL vic_enable
	LDMFD SP!, {R0, R1, R2, PC}

/*
 * Disables IRQ for the specified source on the VIC and SIC
 */
.global disable_irq
.type disable_irq, %function
.align 2
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
	BL vic_disable
	LDMFD SP!, {R0, R1, R2, PC}


/*
 * This function is called whenever an IRQ arrives from the VIC
 */
.global irq_handler
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

.global swi_handler
.align 2
swi_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.global undef_handler
.align 2
undef_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.global prefetch_abort_handler
.align 2
prefetch_abort_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.global data_abort_handler
.align 2
data_abort_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	


.global fiq_handler
.align 2
fiq_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

