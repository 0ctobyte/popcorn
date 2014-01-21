.text
.code 32

/* Important SIC registers */
.align 2
SICBASEADDRESS: .word 0x10003000
SICSTATUS: .word 0x0
SICRAWSTAT: .word 0x4
SICENABLE: .word 0x8
SICENCLR: .word 0xC
SICSOFTINTSET: .word 0x10
SICSOFTINTCLR: .word 0x14
SICPICENABLE: .word 0x20
SICPECENCLR: .word 0x24

/* Initializes the SIC */
.global sic_init
.type sic_init, %function
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

/* Enables an IRQ source on the SIC */
.global sic_enable
.type sic_enable, %function	
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

/* Disables an IRQ source on the SIC */
.global sic_disable
.type sic_disable, %function
.align 2
sic_disable:
	STMFD SP!, {R0, R1, LR}

	/* Write the bit to the SICENCLR register which will then clear the bit */
	/* in the SICENABLE register */
	LDR R1, SICENCLR
	MOV R0, R0
	STR R0, [R1]

	LDMFD SP!, {R0, R1, PC}

/* Gets the SICSTATUS register */
.global sic_status
.type sic_status, %function
.align 2
sic_status:
	STMFD SP!, {R1, LR}

	/* Get the contents of the SICSTATUS register */
	LDR R1, SICSTATUS
	MOV R0, #0
	LDR R0, [R1]

	LDMFD SP!, {R1, PC}

/* This function is the ISR for the SIC */
.global sic_isr
.type sic_isr, %function
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

