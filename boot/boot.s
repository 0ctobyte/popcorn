.text
# Using the ARM instruction set rather than Thumb
.code 32 
.global _start

# Setup the stacks in the BSS. I like my stacks page-aligned pls
# TODO: Should probably make stacks bigger/flexible size
.comm __svc_stack_limit, 4096, 4096
.comm __abt_stack_limit, 4096, 4096
.comm __irq_stack_limit, 4096, 4096
.comm __fiq_stack_limit, 4096, 4096
.comm __und_stack_limit, 4096, 4096
.comm __mon_stack_limit, 4096, 4096

# Setup the system dependent variables in the BSS
.comm PAGESIZE, 4, 4
.comm MEMSIZE, 4, 4
.comm MEMBASEADDR, 4, 4

# Align to 4 byte (word) boundary, the 2 specifies the # of zeros in the low
# order bits
.align 2 
_start:
	# Set the svc stack
	LDR SP, =__svc_stack_limit+4096

	# Get the program status register	
	MRS R0, CPSR

	# Set IRQ mode stack, SP (and LR) is banked in IRQ mode
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x12
	MSR CPSR, R1
	LDR SP, =__irq_stack_limit+4096

	# Set up the FIQ mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x11
	MSR CPSR, R1
	LDR SP, =__fiq_stack_limit+4096
	
	# Set up the Abort mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x17
	MSR CPSR, R1
	LDR SP, =__abt_stack_limit+4096

	# Set up the Undefined mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x1B
	MSR CPSR, R1
	LDR SP, =__und_stack_limit+4096

	# Set up the Monitor mode stack
	BIC R1, R0, #0x1F
	ORR R1, R1, #0x16
	MSR CPSR, R1
	LDR SP, =__mon_stack_limit+4096

	# Go back to supervisor mode
	MSR CPSR, R0

  # CPACR: Allow full (PL0 & PL1) access to coprocessors 10 & 11 (VFPU) 
  MRC p15, 0, R2, c1, c0, 2
  MOV R3, #0xF
  ORR R2, R2, R3, LSL #20
  MCR p15, 0, R2, c1, c0, 2

  # Enable the VFPU
  VMRS R2, FPEXC
  MOV R3, #1
  ORR R2, R2, R3, LSL #30
  VMSR FPEXC, R2

	# Jump to kernel main
	BL kmain

	# In case kmain returns
	B .

