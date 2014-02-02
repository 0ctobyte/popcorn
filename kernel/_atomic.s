.text
.code 32

# Atomic test and set, test fails if value is NOT zero
# R0 [in] - Memory location of value to atomically test and set
# R1 [in] - Value to set
# R0 [out] - 0 on success, otherwise failure
.global atomic_test_and_set
.type atomic_test_and_set, %function
.align 2
atomic_test_and_set:
	STMFD SP!, {R1, R2, LR}	

	MOV R2, R1
	MOV R1, R0
	LDREX R0, [R1]
	CMP R0, #0
	STREXEQ R0, R2, [R1]

	LDMFD SP!, {R1, R2, PC}

# Atomically test and set specified bit, test fails if bit is not zero
# R0 [in] - Memory location of value to atomically test and set
# R1 [in] - Bit to set
# R0 [out] - 0 on success, otherwise failure
.global atomic_test_and_set_bit
.type atomic_test_and_set_bit, %function
.align 2
atomic_test_and_set_bit:
	STMFD SP!, {R1, R2, R3, LR}

	MOV R2, R1
	MOV R1, R0
	LDREX R0, [R1]
	ANDS R3, R0, R2
	ORREQ R2, R0, R2
	STREXEQ R0, R2, [R1]
	
	LDMFD SP!, {R1, R2, R3, PC}

