.text
.code 32

UART0BASEADDRESS: .word 0x101F1000
UART0DATA: .word 0x0

/*
 * Prints a character to UART0
 * R0 [in] - Character to print
 * R0 [out] - 0 for success, -1 for failure
 */
.global putchar
.type putchar, %function
putchar:
	STMFD SP!, {R1, R2, LR}

	LDR R1, UART0BASEADDRESS
	LDR R2, UART0DATA
	MOV R0, R0
	STR R0, [R1, R2]
	MOV R0, #0

	LDMFD SP!, {R1, R2, PC}

