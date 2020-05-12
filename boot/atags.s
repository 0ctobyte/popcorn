# ATAG (ARM tags) is structured information provided by a bootloader to the kernel on boot
# The location to these tags are placed in R2 (usually 0x100)
# Follow the link for more info: http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html

.text
.code 32

.global _atagit
.align 2
_atagit:
  # ATAGs located at memory address in R2
  # Get the size of the ATAG structure and ATAG structure ID
  ldr r0, [r2], #4
  ldr r1, [r2], #4

  # Done if we receive the ATAG_NONE ID (0x0)
  teq r1, #0
  beq _atagit_done

  # Read the ATAG_CORE structure
  movw r3, #0x0001
  movt r3, #0x5441
  teq r1, r3
  beq _atagit_core

  # Read the ATAG_MEM structure
  MOVW R3, #0x0002
  MOVT R3, #0x5441
  TEQ R1, R3
  BEQ _atagit_mem

  b _atagit

_atagit_core:
  # Make sure the ATAG_CORE structure isn't empty
  teq r0, #2
  beq _atagit

  # Get the systems page size, we don't care about the flags or the root device number
  ldr r4, [r2, #4]!
  add r2, r2, #8

  b _atagit

_atagit_mem:
  # Make sure the ATAG_MEM structure isn't empty
  teq r0, #2
  beq _atagit

  # Get the system mem size and physical mem start address
  ldr r5, [r2], #4
  ldr r6, [r2], #4

  b _atagit

_atagit_done:
  # Return values: PAGESIZE, MEMSIZE, MEMBASEADDR
  mov r0, r4
  mov r1, r5
  mov r2, r6
  bx lr

