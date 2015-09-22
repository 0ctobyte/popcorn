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
# r12 holds the start address of the kernel virtual address space (0xF0000000)
# r11 holds the physical start address of system DRAM
# r10 holds placement physical address
# r9 holds page directory physical address
# r8 holds physical address to first page table
# r7 holds number of page tables
# r6 holds the page size in bytes
VIRTUALBASEADDR .req r12
MEMBASEADDR .req r11
placement_addr .req r10
pgd_addr .req r9
pgt_start_addr .req r8
pgt_num .req r7
PAGESIZE .req r6

.align 2
_loader:
  # Switch to supervisor mode & disable all interrupts
  cpsid aif, #0x13

#ifdef BBB
  # Disable watchdog timer
  bl _disable_wdt
#endif
 
  # Reset SCTLR such that the I & D cache, branch predictor and MMU are disabled
  mrc p15, 0, r0, c1, c0, 0
  # Clear bit 12 to disable ICache
  bic r0, r0, #0x1000   
  # Clear bit 2 to disable DCache
  bic r0, r0, #0x4  
  # Clear bit 11 to disable branch predictior
  bic r0, r0, #0x0800 
  # Clear bit 0 to disable MMU
  bic r0, r0, #0x1    
  mcr p15, 0, r0, c1, c0, 0
  
  # Invalidate all caches
  bl _tlb_invalidate_all
  bl _icache_invalidate_all
  bl _dcache_invalidate_all
  bl _bp_invalidate_all
 
#ifdef BBB 
  mov r0, #0x1000
  movw r1, #0x0000
  movt r1, #0x2000
  movw r2, #0x0000
  movt r2, #0x8000
#else
  # Read the ATAGs
  bl _atagit
#endif

  # Store the system dependent variables read from the ATAGS
  mov MEMBASEADDR, r2
  ldr r4, =__kernel_virtual_start
  ldr r5, =__kernel_physical_start
  sub r4, r4, r5
  # The kernel virtual address space starts at 0xF0000000
  # The kernel proper doesn't start until 0xF0010000
  mov VIRTUALBASEADDR, r4
  
  ldr r3, =KVIRTUALBASEADDR
  sub r3, r3, VIRTUALBASEADDR
  add r3, r3, MEMBASEADDR
  str r4, [r3]
  
  ldr r3, =PAGESIZE
  sub r3, r3, VIRTUALBASEADDR
  add r3, r3, MEMBASEADDR
  str r0, [r3]
  
  ldr r3, =MEMSIZE
  sub r3, r3, VIRTUALBASEADDR
  add r3, r3, MEMBASEADDR
  str r1, [r3]
  
  ldr r3, =MEMBASEADDR
  sub r3, r3, VIRTUALBASEADDR
  add r3, r3, MEMBASEADDR
  str r2, [r3]

	# Set the svc stack, remember we need to use the loaded physical address
	# Not the virtual address (R11=MEMBASEADDR)
	ldr sp, =__svc_stack_limit+4096
  sub sp, sp, VIRTUALBASEADDR
  add sp, sp, MEMBASEADDR

  # Set the page size register
  mov PAGESIZE, r0

  # 256 page tables == 256 MiB of kernel memory
	movw pgt_num, #0x100
  ldr r0, =NUMPAGETABLES
  sub r0, r0, VIRTUALBASEADDR
  add r0, r0, MEMBASEADDR
  str pgt_num, [r0]

  # Set placement_addr to a 16 KiB boundary after __kernel_physical_end so we can place
  # the page directory there since the page directory needs to be 16 KiB aligned
	ldr placement_addr, =__kernel_physical_end
  # Round down to last 16 KiB boundary
  movw r0, #0x3fff
  bic placement_addr, placement_addr, r0
  # Now add 0x4000 (16384) to get address of next 16 KiB boundary
  movw r0, #0x4000
  add placement_addr, placement_addr, r0
  add placement_addr, placement_addr, membaseaddr
  
	# This works because ARM uses PC relative addressing
	# Create page dir at end of kernel
	bl _create_page_dir
  ldr r0, =PGDPHYSICALBASEADDR
  sub r0, r0, VIRTUALBASEADDR
  add r0, r0, MEMBASEADDR
  str pgd_addr, [r0]

	# Page tables will be created right after the page directory
	mov r0, pgt_num
	bl _create_page_tables
  ldr r0, =PGTPHYSICALSTARTADDR
  sub r0, r0, VIRTUALBASEADDR
  add r0, r0, MEMBASEADDR
  str pgt_start_addr, [r0]

	bl _setup_page_dir

	bl _do_mapping

	bl _enable_mmu

  # Now we start the kernel proper
  ldr r0, =_start
  bx r0

# This routine sets up registers which will be used when paging is enabled
.align 2
_enable_mmu:
	# Setup the domain access control register
	movw r0, #0x500d
	movt r0, #0xff55
	mcr p15, 0, r0, c3, c0, 0

	# System control register. Enable access flag, Tex remap
	mov r1, #3
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, r1, lsl #28
	mcr p15, 0, r0, c1, c0, 0

	# Set N to zero in Translation Table Base Control Register
	mrc p15, 0, r0, c2, c0, 2
	bic r0, r0, #7
	mcr p15, 0, r0, c2, c0, 2

	# Set the address of the page directory in the translation table base
	# register 0
	mov r0, pgd_addr
	mcr p15, 0, r0, c2, c0, 0

	# Setup the secure configuration register
	#mrc p15, 0, r0, c1, c1, 0
	bic r0, r0, #1
	#mcr p15, 0, r0, c1, c1, 0

	# Setup the primary region remap register
	movw r0, #0x8aa4
	movt r0, #0xf009
	# First check if multiple shareability domains are implemented in ID_MMFR0
	mrc p15, 0, r1, c0, c1, 4
	lsr r1, r1, #12
	and r1, r1, #0xf
	teq r1, #1
	beq a0
	# Only one level, don't need NOSn bits
	bic r0, r0, #0xff000000
a0:
	mcr p15, 0, r0, c10, c2, 0

  # Setup the normal memory remap register
	movw r0, #0x48e0
	movt r0, #0x44e0
	mcr p15, 0, r0, c10, c2, 1

#ifdef BBB
  # Disable Normal shareable memory in the PRRR
  mrc p15, 0, r0, c10, c2, 0
  movw r1, #0x0000
  movt r1, #0x000f
  bic r0, r0, r1
  movw r1, #0x3
  bfi r0, r1, #16, #4
  mcr p15, 0, r0, c10, c2, 0
#endif

	# Now we enable the MMU
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #1
	mcr p15, 0, r0, c1, c0, 0

  bx lr

# Maps the kernel to 0xF0010000, identity maps the first 1 Mb (for the loader)
.align 2
_do_mapping:
	stmfd sp!, {r4, lr}

	# First we need to setup the page table entry descriptor
  # Attributes:
  # - Normal Memory
  # - Inner & Outer Cacheable
  # - R/W permissions only in PL1
  # - Inner Shareable
  # - Global (not flushed from cache on context switch)
	movw r0, #0x45e

	# Now map the kernel (.text & .data section)
	ldr r1, =__kernel_virtual_start
	ldr r2, =__kernel_physical_start
  add r2, r2, MEMBASEADDR
	ldr r3, =__kernel_virtual_end

	bl _map_page_range

	# Now map the page directory and page tables
	movw r0, #0x45e
	sub r1, pgd_addr, MEMBASEADDR
  add r1, r1, VIRTUALBASEADDR
	mov r2, pgd_addr 
  movw r3, #0x400
  mul r3, r3, pgt_num
  add r3, r3, pgt_start_addr
  sub r3, r3, MEMBASEADDR
  add r3, r3, VIRTUALBASEADDR

	bl _map_page_range
 
#ifdef BBB 
  movw r0, #0x142e
  movt r0, #0x1
  movw r1, #0x0000
  movt r1, #0x9f70
  mov r2, r1
  bl _map_section
#endif

	# Identity map the first 1 MiB so when we switch to virtual memory mode we don't 
  # encounter instruction prefetch or data aborts
  # The page directory entry represents a section entry with the following attributes:
  # - Normal Memory 
  # - Inner & Outer cacheable
  # - R/W permissions only in PL1
  # - Inner Shareable
  # - Global
  # - Non-secure memory 
	movw r0, #0x142e
	movt r0, #0x1
  mov r1, MEMBASEADDR
	mov r2, r1

	bl _map_section

	ldmfd sp!, {r4, lr}
  bx lr

# Maps a single section appropriately
# R0 [in] - The section descriptor
# R1 [in] - A section-aligned virtual address
# R2 [in] - A section-aligned physical address
.align 2
_map_section:
	# Construct the section descriptor
	orr r0, r0, r2

	# Get the address into the page dir where the section descriptor will be stored
	lsr r1, r1, #18
	bic r1, r1, #3
	orr r2, r1, pgd_addr

	# Write the section descriptor to the page directory
	str r0, [r2]

  bx lr

# Maps a single page in the appropriate page table in the page directory
# Takes as input a page table entry descriptor in R0, a virtual page-aligned
# address in R1, and the physical page-aligned address in R2.
# Note that all addresses are expected to be page aligned and the top 20 bits
# of the page table entry descriptor are expected to be zero, otherwise the
# results of this routine are undefined.
.align 2
_map_page:
	stmfd sp!, {r4}

	# Concatenate the physical page-aligned address with the page table
	# entry descriptor. This value will be placed in the page table
	orr r0, r0, r2

	# Get the entry in the page directory
	orr r3, pgd_addr, r1, lsr #18
	bic r3, r3, #3
	ldr r4, [r3]

	# Get the address into the page table; this is where the page table
	# entry will be placed
  movw r2, #0x0fff
  movt r2, #0xfff0
	bic r3, r1, r2
	movw r2, #0x3ff
	bic r4, r4, r2
	orr r3, r4, r3, lsr #10
	
	# Place the mapping into the entry in the page table overwriting the
	# entry if one is already present
	str r0, [r3]

	ldmfd sp!, {r4}
  bx lr

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
	stmfd sp!, {lr}

_map_page_range_loop:
	cmp r1, r3
	bpl _map_page_range_end

  stmfd sp!, {r0, r1, r2, r3}
	bl _map_page
  ldmfd sp!, {r0, r1, r2, r3}

	# Increment by page size
	add r1, r1, PAGESIZE
	add r2, r2, PAGESIZE

	b _map_page_range_loop

_map_page_range_end:
	ldmfd sp!, {lr}
  bx lr

# Sets up the page directory entries
.align 2
_setup_page_dir:
	stmfd sp!, {r4}

	mov r0, pgt_start_addr
	
	# Number of entries in the page dir is 4096
	mov r1, #4096
	mov r2, pgt_num

	# The last pgt_num megabytes are mapped to the kernel
	# One entry is 4 bytes long
	# R1 holds the address in the page directory where the page table
	# entries will begin
	sub r1, r1, r2
	lsl r1, r1, #2
	add r1, r1, pgd_addr

	# Set domain field to #1 and 1st bit is #1 to indicate page table entry
	mov r3, #0x21

_setup_page_dir_loop:
	# Decrement the counter
	# Exit loop if counter is < 0
	subs r2, r2, #1
	bmi _setup_page_dir_exit

	# Construct page directory entry
	orr r4, r0, r3

	# Insert entry into page directory and increment R1 by 4
	str r4, [r1], #4

	# Get address of next page table
	add r0, #1024
	
	b _setup_page_dir_loop

_setup_page_dir_exit:
	ldmfd sp!, {r4}
  bx lr

# Create a page directory (1st level page table)
.align 2
_create_page_dir:
	stmfd sp!, {lr}	

	# Allocate 16 kb for page directory
	mov r0, #16	
	bl _alloc
	
	# Put that shit in pgd_addr
	mov pgd_addr, r0

	# Clear the page directory, there are 4096 entries in page directory
	mov r1, #4096
	mov r2, #0
	bl _memsetw

	ldmfd sp!, {lr}
  bx lr

# Create page tables (2nd level page tables)
# Takes as input the # of tables to create in R0
.align 2
_create_page_tables:
	stmfd sp!, {r4, lr}

	# R0 will be overwritten by _alloc
	mov r4, r0
	
	# Allocate memory for page tables
	bl _alloc

	# Set pgt_start_addr
	mov	pgt_start_addr, r0

	# There are 256 entries (words) in each page table
	# Clear it all
	lsl r1, r4, #8
	mov r2, #0
	bl _memsetw

	ldmfd sp!, {r4, lr}
  bx lr

# Sets a range of words to the specified value
# Takes as input the starting address in R0, number of words in R1
# and value to set in R2
.align 2
_memsetw:
	subs r1, r1, #1
	bmi _memsetw_done

	# Store word in [R0] and increment R0 to next word
	str r2, [r0], #4

	b _memsetw

_memsetw_done:
  bx lr

# Simple allocation using the placement address (assumes this value is
# stored in placement_addr)  as pointer to free block
# Takes as input size of block (in kb) to allocate in R0
# Returns the address of the beginning of the block
.align 2
_alloc:
	# Convert kb to bytes
	lsl r0, r0, #10

	# Increment placement address by block size
	add placement_addr, placement_addr, r0

	# Store starting address of block in R0
	sub r0, placement_addr, r0

  bx lr

.align 2
_tlb_invalidate_all:
  # ITLBIALL
  mcr p15, 0, r0, c8, c5, 0
  # DTLBIALL
  mcr p15, 0, r0, c8, c6, 0
  # TLBIALL
  mcr p15, 0, r0, c8, c7, 0
  # Make sure the invalidation is complete
  dsb
  # Make sure the instruction stream sees the invalidation
  isb

  bx lr

.align 2
_icache_invalidate_all:
  # ICIALLU
  mcr p15, 0, r0, c7, c5, 0
  # BPIALL (branch prediction invalidate)
  mcr p15, 0, r0, c7, c5, 6
  # Make sure the invalidation is complete
  dsb
  # Make sure the instruction stream sees the invalidation
  isb

  bx lr

.align 2
_bp_invalidate_all:
  # Invalidate branch predictor array
  # BPIALL (branch prediction invalidate)
  mcr p15, 0, r0, c7, c5, 6

  bx lr

.align 2
_dcache_invalidate_all:
  # CLIDR - Cache Level ID Register. Get # of levels of caches
  mrc p15, 1, r0, c0, c0, 1
  movw r1, #0x0000
  movt r1, #0x0700
  # Extract coherency level
  and r0, r0, r1
  lsr r0, r0, #24

  # DCISW
  #mcr p15, 0, r0, c7, c6, 2

  bx lr

# Disable the watchdog timer
.align 2
_disable_wdt:
  movw r0, #0x5000
  movt r0, #0x44e3
  movw r2, #0xaaaa
wdt_wpsr_write:
  # Offset to WSPR register (watchdog timer start/stop register) 
  add r1, r0, #0x48
  str r2, [r1]
  # Offset to WWPS register (watchdog timer write posting bits register)
  add r1, r0, #0x34
wdt_wwps_poll:
  ldr r3, [r1]
  # Check if write is pending
  tst r3, #0x10
  bne wdt_wwps_poll
  movw r3, #0x5555
  teq r2, r3
  beq wdt_done
  movw r2, #0x5555
  b wdt_wpsr_write
wdt_done:

  bx lr

