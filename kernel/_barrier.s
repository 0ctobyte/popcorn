# Memory and instruction barriers, these can be called in C code
.text
.code 32

# Data Memory Barrier
.global barrier_dmb
.type barrier_dmb, %function
barrier_dmb:
	STMFD SP!, {LR}
	DMB
	LDMFD SP!, {PC}

# Data Synchronization Barrier
# Ensures all instructions before this instruction complete
# Use this when you need to ensure that memory accesses complete before
# execution of the code resumes
.global barrier_dsb
.type barrier_dsb, %function
barrier_dsb:
	STMFD SP!, {LR}
	DSB
	LDMFD SP!, {PC}

# Instruction Synchronization Barrier
# Flushes the instruction pipeline in the processor
# Use this whenever instructions after the ISB are invalid/changed
.global barrier_isb
.type barrier_isb, %function
barrier_isb:
	STMFD SP!, {LR}
	ISB
	LDMFD SP!, {PC}

