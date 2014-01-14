.text
.code 32 /* Using the ARM instruction set rather than Thumb */

.global _start

/* Setup the stacks in the BSS */
.comm __svc_stack_bottom, 4096, 4
.comm __irq_stack_bottom, 4096, 4

/* Align to 4 byte (word) boundary, the 2 specifies the # of zeros in the low
 * order bits
 */
.align 2 
_start:

	/* Set the svc stack */
	LDR SP, =__svc_stack_bottom+4096
	
	/* Get program status register */
	MRS R0, CPSR
	
	/* Clear mode bits */
	BIC R1, R0, #0x1F
	
	/* Go into IRQ mode */
	ORR R1, R1, #0x12
	
	MSR CPSR, R1
	
	/* Set IRQ mode stack, SP (and LR) is banked in IRQ mode */
	LDR SP, =__irq_stack_bottom+4096

	/* Go back to supervisor mode */
	MSR CPSR, R0
	
	/* Jump to kernel main */
	BL kmain

	/* In case kmain returns */
	B .

