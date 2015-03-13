# This code is the first to run on boot. It sets up the page directory and
# page tables, maps the kernel (starting at physical address 0x10000) to
# 0xF0010000, maps the page directory and page tables to the first 16 kb 
# aligned address after the kernel.
# Identity maps the first 1 Mb. 
# The code then sets up control registers appropriately
# and enables paging. Then a branch is made kernel/boot/boot.s:_start
# The kernel is mapped in the top 256 MB of the virtual address space.

.text
.code 32

.global _loader

# Special use registers
# R12 holds the start address of the kernel virtual address space (0xF0000000)
# R11 holds the physical start address of system DRAM
# R10 holds placement physical address
# R9 holds page directory physical address
# R8 holds physical address to first page table
# R7 holds number of page tables
# R6 holds the page size in bytes
VIRTUALBASEADDR .req R12
MEMBASEADDR .req R11
placement_addr .req R10
pgd_addr .req R9
pgt_start_addr .req R8
pgt_num .req R7
PAGESIZE .req R6

.align 2
_loader:
  # Read the ATAGs
  BL _atagit

  # Store the system dependent variables read from the ATAGS
  MOV MEMBASEADDR, R2
  LDR R4, =__kernel_virtual_start
  LDR R5, =__kernel_physical_start
  SUB R4, R4, R5
  # The kernel virtual address space starts at 0xF0000000
  # The kernel proper doesn't start until 0xF0010000
  MOV VIRTUALBASEADDR, R4
  
  LDR R3, =KVIRTUALBASEADDR
  SUB R3, R3, VIRTUALBASEADDR
  ADD R3, R3, MEMBASEADDR
  STR R4, [R3]
  
  LDR R3, =PAGESIZE
  SUB R3, R3, VIRTUALBASEADDR
  ADD R3, R3, MEMBASEADDR
  STR R0, [R3]
  
  LDR R3, =MEMSIZE
  SUB R3, R3, VIRTUALBASEADDR
  ADD R3, R3, MEMBASEADDR
  STR R1, [R3]
  
  LDR R3, =MEMBASEADDR
  SUB R3, R3, VIRTUALBASEADDR
  ADD R3, R3, MEMBASEADDR
  STR R2, [R3]

	# Set the svc stack, remember we need to use the loaded physical address
	# Not the virtual address (R11=MEMBASEADDR)
	LDR SP, =__svc_stack_limit+4096
  SUB SP, SP, VIRTUALBASEADDR
  ADD SP, SP, MEMBASEADDR

  # Set the page size register
  MOV PAGESIZE, R0

  # 256 page tables == 256 MiB of kernel memory
	MOVW pgt_num, #0x100
  LDR R0, =NUMPAGETABLES
  SUB R0, R0, VIRTUALBASEADDR
  ADD R0, R0, MEMBASEADDR
  STR pgt_num, [R0]

  # Set placement_addr to a 16 KiB boundary after __kernel_physical_end so we can place
  # the page directory there since the page directory needs to be 16 KiB aligned
	LDR placement_addr, =__kernel_physical_end
  # Round down to last 16 KiB boundary
  MOVW R0, #0x3FFF
  BIC placement_addr, placement_addr, R0
  # Now add 0x4000 (16384) to get address of next 16 KiB boundary
  MOVW R0, #0x4000
  ADD placement_addr, placement_addr, R0
  ADD placement_addr, placement_addr, MEMBASEADDR
  
	# This works because ARM uses PC relative addressing
	# Create page dir at end of kernel
	BL _create_page_dir
  LDR R0, =PGDPHYSICALBASEADDR
  SUB R0, R0, VIRTUALBASEADDR
  ADD R0, R0, MEMBASEADDR
  STR pgd_addr, [R0]

	# Page tables will be created right after the page directory
	MOV R0, pgt_num
	BL _create_page_tables
  LDR R0, =PGTPHYSICALSTARTADDR
  SUB R0, R0, VIRTUALBASEADDR
  ADD R0, R0, MEMBASEADDR
  STR pgt_start_addr, [R0]

	BL _setup_page_dir

	BL _do_mapping

	BL _enable_mmu

  # Now we start the kernel proper
  LDR R0, =_start
  BX R0

# This routine sets up registers which will be used when paging is enabled
.align 2
_enable_mmu:
	# Setup the domain access control register
	MOVW R0, #0x500D
	MOVT R0, #0xFF55
	MCR p15, 0, R0, c3, c0, 0

	# System control register. Enable access flag, Tex remap
	MOV R1, #3
	MRC p15, 0, R0, c1, c0, 0
	ORR R0, R0, R1, LSL #28
	MCR p15, 0, R0, c1, c0, 0

	# Set N to zero in Translation Table Base Control Register
	MRC p15, 0, R0, c2, c0, 2
	BIC R0, R0, #7
	MCR p15, 0, R0, c2, c0, 2

	# Set the address of the page directory in the translation table base
	# register 0
	MOV R0, #0
	ORR R0, pgd_addr, #0x2B
	MCR p15, 0, R0, c2, c0, 0

	# Setup the secure configuration register
	MRC p15, 0, R0, c1, c1, 0
	BIC R0, R0, #1
	MCR p15, 0, R0, c1, c1, 0

	# Setup the primary region remap register
	MOVW R0, #0x8AA4
	MOVT R0, #0xF009
	# First check if multiple shareability domains are implemented in ID_MMFR0
	MRC p15, 0, R1, c0, c1, 4
	LSR R1, R1, #12
	AND R1, R1, #0xF
	TEQ R1, #1
	BEQ A0
	# Only one level, don't need NOSn bits
	BIC R0, R0, #0xFF000000
A0:
	MCR p15, 0, R0, c10, c2, 0

	# Setup the normal memory remap register
	MOVW R0, #0x48E0
	MOVT R0, #0x44E0
	MCR p15, 0, R0, c10, c2, 1

	# Now we enable the MMU
	MRC p15, 0, R0, c1, c0, 0
	ORR R0, R0, #1
	MCR p15, 0, R0, c1, c0, 0

  BX LR

# Maps the kernel to 0xF0010000, identity maps the first 1 Mb (for the loader)
.align 2
_do_mapping:
	STMFD SP!, {R4, LR}

	# First we need to setup the page table entry descriptor
  # Attributes:
  # - Normal Memory
  # - Inner & Outer Cacheable
  # - R/W permissions only in PL1
  # - Inner Shareable
  # - Global (not flushed from cache on context switch)
	MOVW R0, #0x44E

	# Now map the kernel (.text & .data section)
	LDR R1, =__kernel_virtual_start
	LDR R2, =__kernel_physical_start
  ADD R2, R2, MEMBASEADDR
	LDR R3, =__kernel_virtual_end

	BL _map_page_range

	# Now map the page directory and page tables
	MOVW R0, #0x44E
	SUB R1, pgd_addr, MEMBASEADDR
  ADD R1, R1, VIRTUALBASEADDR
	MOV R2, pgd_addr 
  MOVW R3, #0x400
  MUL R3, R3, pgt_num
  ADD R3, R3, pgt_start_addr
  SUB R3, R3, MEMBASEADDR
  ADD R3, R3, VIRTUALBASEADDR

	BL _map_page_range

	# Identity map the first 1 MiB so when we switch to virtual memory mode we don't 
  # encounter instruction prefetch or data aborts
  # The page directory entry represents a section entry with the following attributes:
  # - Normal Memory 
  # - Inner & Outer cacheable
  # - R/W permissions only in PL1
  # - Inner Shareable
  # - Global
  # - Non-secure memory 
	MOVW R0, #0x102E
	MOVT R0, #0x1
  MOV R1, MEMBASEADDR
	MOV R2, R1

	BL _map_section

	LDMFD SP!, {R4, LR}
  BX LR

# Maps a single section appropriately
# R0 [in] - The section descriptor
# R1 [in] - A section-aligned virtual address
# R2 [in] - A section-aligned physical address
.align 2
_map_section:
	# Construct the section descriptor
	ORR R0, R0, R2

	# Get the address into the page dir where the section descriptor will be stored
	LSR R1, R1, #18
	BIC R1, R1, #3
	ORR R2, R1, pgd_addr

	# Write the section descriptor to the page directory
	STR R0, [R2]

  BX LR

# Maps a single page in the appropriate page table in the page directory
# Takes as input a page table entry descriptor in R0, a virtual page-aligned
# address in R1, and the physical page-aligned address in R2.
# Note that all addresses are expected to be page aligned and the top 20 bits
# of the page table entry descriptor are expected to be zero, otherwise the
# results of this routine are undefined.
.align 2
_map_page:
	STMFD SP!, {R4}

	# Concatenate the physical page-aligned address with the page table
	# entry descriptor. This value will be placed in the page table
	ORR R0, R0, R2

	# Get the entry in the page directory
	ORR R3, pgd_addr, R1, LSR #18
	BIC R3, R3, #3
	LDR R4, [R3]

	# Get the address into the page table; this is where the page table
	# entry will be placed
  MOVW R2, #0x0FFF
  MOVT R2, #0xFFF0
	BIC R3, R1, R2
	MOVW R2, #0x3FF
	BIC R4, R4, R2
	ORR R3, R4, R3, LSR #10
	
	# Place the mapping into the entry in the page table overwriting the
	# entry if one is already present
	STR R0, [R3]

	LDMFD SP!, {R4}
  BX LR

# Maps a range of continuous physical pages to a range of continuous virtual
# R0 [in] - Page table entry descriptor 
# R1 [in] - Starting virtual address of range
# R2 [in] - Starting physical address of range
# R3 [in] - Ending virtual address of range (This address is NOT mapped)
# Note that all addresses are expected to be page-aligned and the top 20 bits
# of the page table entry descriptor are expected to be zero. Also, the size
# of the virtual address range is expected to be the same as the size of the
# physical address range. If the above conditions are not met, the results of
# this routine are undefined
.align 2
_map_page_range:
	STMFD SP!, {LR}

_map_page_range_loop:
	CMP R1, R3
	BPL _map_page_range_end

  STMFD SP!, {R0, R1, R2, R3}
	BL _map_page
  LDMFD SP!, {R0, R1, R2, R3}

	# Increment by page size
	ADD R1, R1, PAGESIZE
	ADD R2, R2, PAGESIZE

	B _map_page_range_loop

_map_page_range_end:
	LDMFD SP!, {LR}
  BX LR

# Sets up the page directory entries
.align 2
_setup_page_dir:
	STMFD SP!, {R4}

	MOV R0, pgt_start_addr
	
	# Number of entries in the page dir is 4096
	MOV R1, #4096
	MOV R2, pgt_num

	# The last pgt_num megabytes are mapped to the kernel
	# One entry is 4 bytes long
	# R1 holds the address in the page directory where the page table
	# entries will begin
	SUB R1, R1, R2
	LSL R1, R1, #2
	ADD R1, R1, pgd_addr

	# Set domain field to #1 and 1st bit is #1 to indicate page table entry
	MOV R3, #0x21

_setup_page_dir_loop:
	# Decrement the counter
	# Exit loop if counter is < 0
	SUBS R2, R2, #1
	BMI _setup_page_dir_exit

	# Construct page directory entry
	ORR R4, R0, R3

	# Insert entry into page directory and increment R1 by 4
	STR R4, [R1], #4

	# Get address of next page table
	ADD R0, #1024
	
	B _setup_page_dir_loop

_setup_page_dir_exit:
	LDMFD SP!, {R4}
  BX LR

# Create a page directory (1st level page table)
.align 2
_create_page_dir:
	STMFD SP!, {LR}	

	# Allocate 16 kb for page directory
	MOV R0, #16	
	BL _alloc
	
	# Put that shit in pgd_addr
	MOV pgd_addr, R0

	# Clear the page directory, there are 4096 entries in page directory
	MOV R1, #4096
	MOV R2, #0
	BL _memsetw

	LDMFD SP!, {LR}
  BX LR

# Create page tables (2nd level page tables)
# Takes as input the # of tables to create in R0
.align 2
_create_page_tables:
	STMFD SP!, {R4, LR}

	# R0 will be overwritten by _alloc
	MOV R4, R0
	
	# Allocate memory for page tables
	BL _alloc

	# Set pgt_start_addr
	MOV	pgt_start_addr, R0

	# There are 256 entries (words) in each page table
	# Clear it all
	LSL R1, R4, #8
	MOV R2, #0
	BL _memsetw

	LDMFD SP!, {R4, LR}
  BX LR

# Sets a range of words to the specified value
# Takes as input the starting address in R0, number of words in R1
# and value to set in R2
.align 2
_memsetw:
	SUBS R1, R1, #1
	BMI _memsetw_done

	# Store word in [R0] and increment R0 to next word
	STR R2, [R0], #4

	B _memsetw

_memsetw_done:
  BX LR

# Simple allocation using the placement address (assumes this value is
# stored in placement_addr)  as pointer to free block
# Takes as input size of block (in kb) to allocate in R0
# Returns the address of the beginning of the block
.align 2
_alloc:
	# Convert kb to bytes
	LSL R0, R0, #10

	# Increment placement address by block size
	ADD placement_addr, placement_addr, R0

	# Store starting address of block in R0
	SUB R0, placement_addr, R0

  BX LR

