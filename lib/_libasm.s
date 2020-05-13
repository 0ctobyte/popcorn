# This file implements some basic functions that can only be implemented
# through assembly

.text
.code 32

# Unsigned integer modulo
# operand1 % operand2
# r0 [in] - operand1
# r1 [in] - operand2
# r0 [out] - The modulo value
.global _umod
.align 2
_umod:
    vmov s0, r0
    vcvt.f64.u32 d0, s0
    vmov s2, r1
    vcvt.f64.u32 d1, s2

    vdiv.f64 d0, d0, d1
    vcvt.u32.f64 s0, d0

    vmov r2, s0

    mls r0, r1, r2, r0

    bx lr

# Count trailing zeros
# Takes a 32-bit integer in R0
# Returns the number of trailing zeros
.global _ctz
.align 2
_ctz:
    rbit r0, r0
    clz r0, r0

    bx lr

# Count leading zeros
# Takes a 32-bit integer in R0
# Returns the number of leading zeros
.global _clz
.align 2
_clz:
    clz r0, r0

    bx lr

# Reverse bit order
# Takes a 32-bit integer in R0
# Returns the 32-bit integer in reversed bit order
.global _rbit
.align 2
_rbit:
    rbit r0, r0

    bx lr
