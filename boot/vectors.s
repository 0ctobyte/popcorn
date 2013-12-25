/*
 * This file sets up the vector table
 */
.text
.code 32 /* Using the ARM instruction set rather than Thumb */

.global _vectors_start
.global _vectors_end
.global _copy_vectors

_vectors_start:

	LDR PC, reset_handler_addr
	LDR PC, undef_handler_addr
	LDR PC, swi_handler_addr
	LDR PC, prefetch_abort_handler_addr
	LDR PC, data_abort_handler_addr
	B . /* Reserved */
	LDR PC, irq_handler_addr
	LDR PC, fiq_handler_addr

reset_handler_addr: .word _start
undef_handler_addr: .word undef_handler
swi_handler_addr: .word swi_handler
prefetch_abort_handler_addr: .word prefetch_abort_handler
data_abort_handler_addr: .word data_abort_handler
irq_handler_addr: .word irq_handler
fiq_handler_addr: .word fiq_handler

_vectors_end:

_copy_vectors:
	STMFD SP!, {R0, R1, R2, R3, R14}
	
	LDR R0, =_vectors_start /* Vector table source */
	LDR R1, =_vectors_end /* End of vector table source */
	MOV R2, #0x0 /* Vector table destination. ARM expects table to start at address 0x0 in memory */

_copy_vectors_loop:
	LDRB R3, [R0], #1 /* Get byte from source vector table */
	STRB R3, [R2], #1 /* Store byte into destination vector table */
	CMP R0, R1
	BEQ _copy_vectors_loop

	LDMFD SP!, {R0, R1, R2, R3, R15}

