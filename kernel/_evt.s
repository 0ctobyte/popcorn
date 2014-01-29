# This file handles the exception vector table
.text
.code 32

# The functions to call on an exception
.comm evt_table, 6*4, 4

# The vector base register requires that the base address is 32 bit aligned
.align 5
exception_vector_table:
	LDR PC, =reset_exception
	LDR PC, =undefined_exception
	LDR PC, =swi_exception
	LDR PC, =prefetch_abort_exception
	LDR PC, =data_abort_exception
	# Reserved
	B .
	LDR PC, =irq_exception
	LDR PC, =fiq_exception

.align 2
reset_exception:
	B _start

.align 2
undefined_exception:
	STMFD SP!, {R0-R12, LR}
	LDR R0, =evt_table
	LDR R0, [R0]
	TEQ R0, #0
	BEQ A0
	BLX R0
A0:
	LDMFD SP!, {R0-R12, PC}

.align 2
swi_exception:
	STMFD SP!, {R0-R12, LR}
	LDR R0, =evt_table
	LDR R0, [R0, #4]
	TEQ R0, #0
	BEQ A1
	BLX R0
A1:
	LDMFD SP!, {R0-R12, PC}

.align 2
prefetch_abort_exception:
	STMFD SP!, {R0-R12, LR}
	LDR R0, =evt_table
	LDR R0, [R0, #8]
	TEQ R0, #0
	BEQ A2
	BLX R0
A2:
	LDMFD SP!, {R0-R12, LR}
	SUBS PC, LR, #4

.align 2
data_abort_exception:
	STMFD SP!, {R0-R12, LR}
	LDR R0, =evt_table
	LDR R0, [R0, #12]
	TEQ R0, #0
	BEQ A3
	BLX R0
A3:
	LDMFD SP!, {R0-R12, LR}
	SUBS PC, LR, #8

.align 2
irq_exception:
	STMFD SP!, {R0-R12, LR}
	LDR R0, =evt_table
	LDR R0, [R0, #16]
	TEQ R0, #0
	BEQ A4
	BLX R0
A4:
	LDMFD SP!, {R0-R12, LR}
	SUBS PC, LR, #4

.align 2
fiq_exception:
	STMFD SP!, {R0-R7, LR}
	LDR R0, =evt_table
	LDR R0, [R0, #20]
	TEQ R0, #0
	BEQ A5
	BLX R0
A5:
	LDMFD SP!, {R0-R7, LR}
	SUBS PC, LR, #4

.global evt_init
.type evt_init, %function
.align 2
evt_init:
	STMFD SP!, {R0, R1, LR}
	
	# Setup the vector base address register
	LDR R0, =exception_vector_table
	MRC P15, 0, R1, C12, C0, 0
	ORR R1, R1, R0
	MCR P15, 0, R1, C12, C0, 0

	LDMFD SP!, {R0, R1, PC}

# R0 [in] - Exception type
# R1 [in] - Exception handler address
.global evt_register_handler
.type evt_register_handler, %function
.align 2
evt_register_handler:
	STMFD SP!, {R0-R2, LR}

	LDR R2, =evt_table
	LSL R0, R0, #2
	STR R1, [R2, R0]

	LDMFD SP!, {R0-R2, PC}

