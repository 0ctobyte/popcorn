.text
.code 32 /* Using the ARM instruction set rather than Thumb */

.global _start

_start:
	LDR SP, =supervisor_stack_top
	BL _copy_vectors
	
	/* Get program status register */
	MRS R0, CPSR
	
	/* Clear mode bits */
	BIC R1, R0, #0x1F
	
	/* Go into IRQ mode */
	ORR R1, R1, #0x12
	
	MSR CPSR, R1
	
	/* Set IRQ mode stack, SP (and LR) is banked in IRQ mode */
	LDR SP, =irq_stack_top

	/* Go back to supervisor mode */
	MSR CPSR, R0
	
	/* Jump to kernel main */
	BL kmain

	/* In case kmain returns */
	B .

