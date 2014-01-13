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

.type vic_init, %function		/* Initializes VIC */
.type vic_enable, %function		/* Enables an IRQ source on the VIC */
.type vic_disable, %function	/* Disables an IRQ source on the VIC */
.type vic_status, %function		/* Gets the VICSTATUS register */

.type sic_isr, %function
.type sic_init, %function		/* Initializes the SIC */
.type sic_enable, %function		/* Enables an IRQ source on the SIC */
.type sic_disable, %function	/* Disables an IRQ source on the SIC */
.type sic_status, %function		/* Gets the SICSTATUS register */

.global vectors_install
.type vectors_install, %function

.global irq_handler
.global swi_handler
.global undef_handler
.global prefetch_abort_handler
.global data_abort_handler
.global fiq_handler


/* Important VIC registers */
.align 2
VICIRQSTATUS: .word 0x10140000 /* VICBASEADDRESS */
VICFIQSTATUS: .word 0x10140004
VICRAWINTR: .word 0x10140008
VICINTSELECT: .word 0x1014000C
VICINTENABLE: .word 0x10140010
VICINTENCLEAR: .word 0x10140014
VICSOFTINT: .word 0x10140018
VICSOFTINTCLEAR: .word 0x1014001C
VICPROTECTION: .word 0x10140020

/* Important SIC registers */
.align 2
SICSTATUS: .word 0x10003000 /* SICBASEADDRESS */
SICRAWSTAT: .word 0x10003004
SICENABLE: .word 0x10003008
SICENCLR: .word 0x1000300C
SICSOFTINTSET: .word 0x10003010
SICSOFTINTCLR: .word 0x10003014
SICPICENABLE: .word 0x10003020
SICPECENCLR: .word 0x10003024


.align 2
_vectors_start:

	LDR PC, reset_handler_addr
	LDR PC, undef_handler_addr
	LDR PC, swi_handler_addr
	LDR PC, prefetch_abort_handler_addr
	LDR PC, data_abort_handler_addr
	B . /* Reserved */
	LDR PC, irq_handler_addr
	LDR PC, fiq_handler_addr


.align 2
reset_handler_addr: .word _start
undef_handler_addr: .word undef_handler
swi_handler_addr: .word swi_handler
prefetch_abort_handler_addr: .word prefetch_abort_handler
data_abort_handler_addr: .word data_abort_handler
irq_handler_addr: .word irq_handler
fiq_handler_addr: .word fiq_handler

_vectors_end:

.align 2
vectors_install:
	STMFD SP!, {R0, R1, R2, R3, LR}
	
	LDR R0, =_vectors_start /* Vector table source */
	LDR R1, =_vectors_end /* End of vector table source */
	MOV R2, #0x0 /* Vector table destination. ARM expects table to start at address 0x0 in memory */

F0:
	LDR R3, [R0], #1 /* Get word from source vector table */
	MOV R2, R2
	STR R3, [R2], #1 /* Store word into destination vector table */
	CMP R0, R1
	BNE F0

	LDMFD SP!, {R0, R1, R2, R3, PC}

/* 
 * This symbol will point to the ISR table with a total of 64 entries
 */
.align 2
isr_table:
	.skip 64*4, 0

.align 2
vic_init:
	STMFD SP!, {R0, R1, LR}

	/* Clear registers first */
	LDR R1, VICINTSELECT
	MOV R0, #0
	STR R0, [R1]
	LDR R1, VICINTENABLE
	MOV R0, #0
	STR R0, [R1]
	LDR R1, VICSOFTINT
	MOV R0, #0
	STR R0, [R1]

	/* Enable protection */
	/*LDR R1, VICPROTECTION
	*MOV R0, #1
	*STR R0, [R1]
	*/

	LDMFD SP!, {R0, R1, PC}

.align 2
vic_enable:
	STMFD SP!, {R0, R1, R2, LR}

	/* Set the bit in the VICINTENABLE register */
	LDR R1, VICINTENABLE /* LDR takes 2 cycles for R1 to be ready */
	MOV R0, R0 /* Gotta avoid those pipeline stalls */
	LDR R2, [R1]
	LDR R2, [R1]
	ORR R0, R2, R0
	STR R0, [R1]

	LDMFD SP!, {R0, R1, R2, PC}

.align 2
vic_disable:
	STMFD SP!, {R0, R1, LR}

	/* Write the bit to VICINTENCLEAR register which will then clear the bit */
	/* in the VICINTENABLE register */
	LDR R1, VICINTENCLEAR
	MOV R0, R0
	STR R0, [R1]
	
	LDMFD SP!, {R0, R1, PC}

.align 2
vic_status:
	STMFD SP!, {R1, LR}

	/* Get the contents of the VICIRQSTATUS register */
	LDR R1, VICIRQSTATUS
	MOV R0, #0
	LDR R0, [R1]

	LDMFD SP!, {R1, PC}

.align 2
sic_init:
	STMFD SP!, {R0, R1, LR}

	/* Clear registers */
	LDR R1, SICENABLE
	MOV R0, #0
	STR R0, [R1]
	LDR R1,  SICSOFTINTSET
	MOV R0, #0
	STR R0, [R1]
	LDR R1, SICPICENABLE
	MOV R0, #0
	STR R0, [R1]

	LDMFD SP!, {R0, R1, PC}

.align 2
sic_enable:
	STMFD SP!, {R0, R1, R2, LR}

	/* Set the bit in the SICENABLE register */
	LDR R1, SICENABLE
	MOV R0, R0
	LDR R2, [R1]
	LDR R2, [R1]
	ORR R0, R2, R0
	STR R0, [R1]

	LDMFD SP!, {R0, R1, R2, PC}

.align 2
sic_disable:
	STMFD SP!, {R0, R1, LR}

	/* Write the bit to the SICENCLR register which will then clear the bit */
	/* in the SICENABLE register */
	LDR R1, SICENCLR
	MOV R0, R0
	STR R0, [R1]

	LDMFD SP!, {R0, R1, PC}

.align 2
sic_status:
	STMFD SP!, {R1, LR}

	/* Get the contents of the SICSTATUS register */
	LDR R1, SICSTATUS
	MOV R0, #0
	LDR R0, [R1]

	LDMFD SP!, {R1, PC}

/*
 * Enable interrupts on the processor
 * Both IRQ and FIQ interrupts are enabled
 */
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
 * This function is the ISR for the SIC
 */
.align 2
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
	ADD R1, R1, #1
	ADD R2, R2, #1
	TEQ R6, #0 /* Check if address is valid, i.e. not NULL */
	BEQ D0
	BLX R6
	B D0
D2:
	LDMFD SP!, {R0-R6, PC}

/*
 * This function is called whenever an IRQ arrives from the VIC
 */
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

.align 2
swi_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.align 2
undef_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.align 2
prefetch_abort_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.align 2
data_abort_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

.align 2
fiq_handler:
	STMFD SP!, {R0-R11}
	LDMFD SP!, {R0-R11}
	B .
	SUBS PC, LR, #4	

