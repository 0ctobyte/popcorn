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
	mov r2, r1
	mov r1, r0
	ldrex r0, [r1]
	teq r0, #0
	strexeq r0, r2, [r1]
  clrex

  bx lr

# Atomically test and set specified bit, test fails if bit is not zero
# R0 [in] - Memory location of value to atomically test and set
# R1 [in] - Bit to set
# R0 [out] - 0 on success, otherwise failure
.global atomic_test_and_set_bit
.type atomic_test_and_set_bit, %function
.align 2
atomic_test_and_set_bit:
	mov r2, r1
	mov r1, r0
	ldrex r0, [r1]
	tst r0, r2
	orreq r2, r0, r2
	strexeq r0, r2, [r1]
  clrex
	
  bx lr

# Atomically increment the value
# R0 [in] - Memory location of atomic value
.global atomic_inc
.type atomic_inc, %function
.align 2
atomic_inc:
  ldrex r1, [r0]
  add r1, r1, #1
  strex r2, r1, [r0]
  teq r2, #0
  bne atomic_inc

  bx lr

# Atomically decrement the value
# R0 [in] - Memory location of atomic value
.global atomic_dec
.type atomic_dec, %function
.align 2
atomic_dec:
  ldrex r1, [r0]
  sub r1, r1, #1
  strex r2, r1, [r0]
  teq r2, #0
  bne atomic_dec

  bx lr

