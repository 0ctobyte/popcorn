.text
# Using the ARM instruction set rather than Thumb
.code 32 
.global _start

# Setup the stacks in the BSS
.comm __svc_stack_bottom, 4096, 4
.comm __abt_stack_bottom, 4096, 4
.comm __irq_stack_bottom, 4096, 4
.comm __fiq_stack_bottom, 4096, 4
.comm __und_stack_bottom, 4096, 4
.comm __mon_stack_bottom, 4096, 4

# Align to 4 byte (word) boundary, the 2 specifies the # of zeros in the low
# order bits
.align 2 
_start:
	# Set the svc stack
	LDR SP, =__svc_stack_bottom+4096

	# Get the program status register	
	MRS R0, CPSR

	# Set IRQ mode stack, SP (and LR) is banked in IRQ mode
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x12
	MSR CPSR, R1
	LDR SP, =__irq_stack_bottom+4096

	# Set up the FIQ mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x11
	MSR CPSR, R1
	LDR SP, =__fiq_stack_bottom+4096
	
	# Set up the Abort mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x17
	MSR CPSR, R1
	LDR SP, =__abt_stack_bottom+4096

	# Set up the Undefined mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x1B
	MSR CPSR, R1
	LDR SP, =__und_stack_bottom+4096

	# Set up the Monitor mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x16
	MSR CPSR, R1
	LDR SP, =__mon_stack_bottom+4096

	# Go back to supervisor mode
	MSR CPSR, R0

	# Jump to kernel main
	BL kmain

	# In case kmain returns
	B .

