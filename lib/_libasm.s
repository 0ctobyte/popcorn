# This file implements some basic functions that can only be implemented through assembly
.text

# Unsigned integer modulo
# operand1 % operand2
# r0 [in] - operand1
# r1 [in] - operand2
# r0 [out] - The modulo value
.global _umod
.align 2
_umod:
    udiv x2, x0, x1
    msub x0, x2, x1, x0
    ret lr

# Count trailing zeros
# Takes a 32-bit integer in R0
# Returns the number of trailing zeros
.global _ctz
.align 2
_ctz:
    rbit x0, x0
    clz x0, x0
    ret lr

# Count leading zeros
# Takes a 32-bit integer in R0
# Returns the number of leading zeros
.global _clz
.align 2
_clz:
    clz x0, x0
    ret lr

# Reverse bit order
# Takes a 32-bit integer in R0
# Returns the 32-bit integer in reversed bit order
.global _rbit
.align 2
_rbit:
    rbit x0, x0
    ret lr
