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
	MOV R2, R1
	MOV R1, R0
	LDREX R0, [R1]
	CMP R0, #0
	STREXEQ R0, R2, [R1]
  CLREX

  BX LR

# Atomically test and set specified bit, test fails if bit is not zero
# R0 [in] - Memory location of value to atomically test and set
# R1 [in] - Bit to set
# R0 [out] - 0 on success, otherwise failure
.global atomic_test_and_set_bit
.type atomic_test_and_set_bit, %function
.align 2
atomic_test_and_set_bit:
	MOV R2, R1
	MOV R1, R0
	LDREX R0, [R1]
	ANDS R3, R0, R2
	ORREQ R2, R0, R2
	STREXEQ R0, R2, [R1]
  CLREX
	
  BX LR

# Atomically set to the specified value
# R0 [in] - Memory location of atomic value
# R1 [in] - Value to set
.global atomic_set
.type atomic_set, %function
.align 2
atomic_set:
  LDREX R2, [R0]
  STREX R2, R1, [R0]
  CMP R2, #0
  BNE atomic_set

  BX LR

# Atomically increment the value
# R0 [in] - Memory location of atomic value
.global atomic_inc
.type atomic_inc, %function
.align 2
atomic_inc:
  LDREX R1, [R0]
  ADD R1, R1, #1
  STREX R2, R1, [R0]
  CMP R2, #0
  BNE atomic_inc

  BX LR

# Atomically decrement the value
# R0 [in] - Memory location of atomic value
.global atomic_dec
.type atomic_dec, %function
.align 2
atomic_dec:
  LDREX R1, [R0]
  SUB R1, R1, #1
  STREX R2, R1, [R0]
  CMP R2, #0
  BNE atomic_dec

  BX LR

