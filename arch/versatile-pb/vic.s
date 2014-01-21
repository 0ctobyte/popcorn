.text
.code 32

/* Important VIC registers */
/* 0x10140000 */
.align 2
VICBASEADDRESS: .word 0xFFFF0000
VICIRQSTATUS: .word 0x0 
VICFIQSTATUS: .word 0x4
VICRAWINTR: .word 0x8
VICINTSELECT: .word 0xC
VICINTENABLE: .word 0x10
VICINTENCLEAR: .word 0x14
VICSOFTINT: .word 0x18
VICSOFTINTCLEAR: .word 0x1C
VICPROTECTION: .word 0x20

/* Initializes VIC */
.global vic_init
.type vic_init, %function
.align 2
vic_init:
	STMFD SP!, {R0, R1, R2, LR}

	/* Clear registers first */
	LDR R0, VICBASEADDRESS
	LDR R1, VICINTSELECT
	MOV R2, #0
	STR R2, [R0, R1]
	LDR R1, VICINTENABLE
	MOV R2, #0
	STR R2, [R0, R1]
	LDR R1, VICSOFTINT
	MOV R2, #0
	STR R2, [R0, R1]

	LDMFD SP!, {R0, R1, R2, PC}

/* Enables an IRQ source on the VIC */
.global vic_enable
.type vic_enable, %function
.align 2
vic_enable:
	STMFD SP!, {R0, R1, R2, R3, LR}

	/* Set the bit in the VICINTENABLE register */
	LDR R3, VICBASEADDRESS
	LDR R1, VICINTENABLE /* LDR takes 2 cycles for R1 to be ready */
	MOV R0, R0 /* Gotta avoid those pipeline stalls */
	LDR R2, [R3, R1]
	LDR R2, [R3, R1]
	ORR R0, R2, R0
	STR R0, [R3, R1]

	LDMFD SP!, {R0, R1, R2, R3, PC}

/* Disables an IRQ source on the VIC */
.global vic_disable
.type vic_disable, %function
.align 2
vic_disable:
	STMFD SP!, {R0, R1, R2, LR}

	/* Write the bit to VICINTENCLEAR register which will then clear the bit */
	/* in the VICINTENABLE register */
	LDR R2, VICBASEADDRESS
	LDR R1, VICINTENCLEAR
	MOV R0, R0
	STR R0, [R2, R1]
	
	LDMFD SP!, {R0, R1, R2, PC}

/* Gets the VICSTATUS register */
.global vic_status
.type vic_status, %function
.align 2
vic_status:
	STMFD SP!, {R1, R2, LR}

	/* Get the contents of the VICIRQSTATUS register */
	LDR R2, VICBASEADDRESS
	LDR R1, VICIRQSTATUS
	MOV R0, #0
	LDR R0, [R2, R1]

	LDMFD SP!, {R1, R2, PC}

