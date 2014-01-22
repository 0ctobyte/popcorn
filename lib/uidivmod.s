/* Integer division and modulo */
.text
.code 32

.global uidivmod
.type uidivmod, %function

/* 
 * Expects two arguments, R0 divided by R1
 * R2 and R3 will hold the address of the locations where the quotient and 
 * remainder will be stored respectively
 * Returns 0 if successful, otherwise 1
 * All values are assumed to be unsigned integers
 * Uses algorithm I found from here:
 * http://en.wikipedia.org/wiki/Division_algorithm#Integer_division_.28unsigned.29_with_remainder 
 */
.align 2 
uidivmod:
	STMFD SP!, {R1, R2, R3, R4, R5, R6, R7, R8, LR}

	CMP R1, #0 /* Check if divisor is 0 */
	BEQ A3

	MOV R4, #0 /* Quotient will be stored here temporarily */
	MOV R5, #0 /* Remainder temp register */
	MOV R6, #31 /* Loop counter, only loop 32 times for 32 bits */
	MOV R7, #1 /* i bit */
	LSL R7, R7, #31

A0:
	LSL R5, R5, #1 /* Shift Remainder right by 1 */
	AND R8, R0, R7 
	LSR R8, R8, R6
	BIC R5, R5, #1
	ORR R5, R5, R8 /* Set LSB of Remainder to bit i of Numerator */
	CMP R5, R1 /* Check if Remainder is greater than Denominator */
	BMI A1
	SUB R5, R5, R1 /* If so, set Remainder as Remainder minus Denominator */
	ORR R4, R4, R7  /* Set bit i of Quotient to 1 */
A1:
	LSR R7, R7, #1
	SUBS R6, R6, #1
	BMI A2
	B A0

A2:
	/* Assuming R2 and R3 are pointers */
	CMP R2, #0 /* Address must not be 0x0 (NULL) */
	BEQ A3
	CMP R3, #0
	BEQ A3

	/* Store the values */
	STR R4, [R2]
	STR R5, [R3]
	
	/* Successful */
	MOV R0, #0
	LDMFD SP!, {R1, R2, R3, R4, R5, R6, R7, R8, PC}

A3:
	MOV R0, #1
	LDMFD SP!, {R1, R2, R3, R4, R5, R6, R7, R8, PC}

