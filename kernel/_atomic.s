.text
.code 32

# Atomic test and set
# R0 [in] - Memory location of value to atomically test and set
# R0 [out] - 0 on success, 1 on failure
.global atomic_test_and_set
.type atomic_test_and_set, %function
atomic_test_and_set:
	STMFD SP!, {LR}	

	MOV R2, #1
	MOV R1, R0
	LDREX R0, [R1]
	CMP R0, #0
	STREXEQ R0, R2, [R1]
	
	LDMFD SP!, {PC}

