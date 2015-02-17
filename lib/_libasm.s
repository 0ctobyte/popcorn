# This file implements some basic functions that can only be implemented
# through assembly

.text
.code 32

# Unsigned integer modulo
# operand1 % operand2
# R0 [in] - operand1
# R1 [in] - operand2
# R0 [out] - The modulo value
.global _umod
.type _umod, %function
.align 2
_umod:
  VMOV S0, R0
  VCVT.F64.U32 D0, S0
  VMOV S2, R1
  VCVT.F64.U32 D1, S2

  VDIV.F64 D0, D0, D1
  VCVT.U32.F64 S0, D0

  VMOV R2, S0

  MLS R0, R1, R2, R0

  BX LR

# Count trailing zeros
# Takes a 32-bit integer in R0
# Returns the number of trailing zeros
.global _ctz
.type _ctz, %function
.align 2
_ctz:
  RBIT R0, R0
  CLZ R0, R0
  
  BX LR

# Count leading zeros
# Takes a 32-bit integer in R0
# Returns the number of leading zeros
.global _clz
.type _clz, %function
.align 2
_clz:
	CLZ R0, R0

  BX LR

# Reverse bit order
# Takes a 32-bit integer in R0
# Returns the 32-bit integer in reversed bit order
.global _rbit
.type _rbit, %function
.align 2
_rbit:
	RBIT R0, R0

  BX LR

