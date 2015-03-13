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
  LDR R0, [R2], #4 
  LDR R1, [R2], #4

  # Done if we receive the ATAG_NONE ID (0x0)
  TEQ R1, #0
  BEQ _atagit_done

  # Read the ATAG_CORE structure
  MOVW R3, #0x0001
  MOVT R3, #0x5441
  TEQ R1, R3
  BEQ _atagit_core

  # Read the ATAG_MEM structure
  MOVW R3, #0x0002
  MOVT R3, #0x5441
  TEQ R1, R3
  BEQ _atagit_mem

  B _atagit

_atagit_core:
  # Make sure the ATAG_CORE structure isn't empty
  TEQ R0, #2
  BEQ _atagit

  # Get the systems page size, we don't care about the flags or the root device number
  LDR R4, [R2, #4]!
  ADD R2, R2, #8

  B _atagit

_atagit_mem:
  # Make sure the ATAG_MEM structure isn't empty
  TEQ R0, #2
  BEQ _atagit

  # Get the system mem size and physical mem start address
  LDR R5, [R2], #4
  LDR R6, [R2], #4

  B _atagit

_atagit_done:
  # Return values: PAGESIZE, MEMSIZE, MEMBASEADDR
  MOV R0, R4
  MOV R1, R5
  MOV R2, R6
  BX LR

