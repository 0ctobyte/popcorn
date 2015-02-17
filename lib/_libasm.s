# This file implements some basic functions that can only be implemented
# through assembly

.text
.code 32

# Count trailing zeros
# Takes a 32-bit integer in R0
# Returns the number of trailing zeros
.global _ctz
.type _ctz, %function
.align 2
_ctz:
  STMFD SP!, {LR}
  RBIT R0, R0
  CLZ R0, R0
  LDMFD SP!, {PC}

# Count leading zeros
# Takes a 32-bit integer in R0
# Returns the number of leading zeros
.global _clz
.type _clz, %function
.align 2
_clz:
	STMFD SP!, {LR}
	CLZ R0, R0
	LDMFD SP!, {PC}

# Reverse bit order
# Takes a 32-bit integer in R0
# Returns the 32-bit integer in reversed bit order
.global _rbit
.type _rbit, %function
.align 2
_rbit:
	STMFD SP!, {LR}
	RBIT R0, R0
	LDMFD SP!, {PC}

