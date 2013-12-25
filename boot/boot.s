.text
.code 32 /* Using the ARM instruction set rather than Thumb */

.global _start

_start:

	LDR SP, =supervisor_stack_top
	BL _copy_vectors
	
	MRS R0, CPSR
	
	/* Get program status register */
	BIC R1, R0, #0x1F
	
	/* Clear mode bits */
	ORR R1, R1, #0x12
	
	/* Go into IRQ mode */
	MSR CPSR, R1
	
	/* Set IRQ mode stack, SP is banked in IRQ mode */
	LDR SP, =irq_stack_top

	/* Enable IRQ's */
	BIC R0, R0, #0x80

	/* Go back to supervisor mode */
	MSR CPSR, R0
	
	/* Jump to kernel main */
	BL kmain
	B .

