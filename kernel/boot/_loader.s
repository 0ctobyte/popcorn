.text
.code 32

.global _loader

/* 
 * Special use registers
 * R10 holds placement address
 * R9 holds page directory address
 * R8 holds address to first page table
 * R7 holds number of page tables (512)
 */
placement_addr .req R10
pgd_addr .req R9
pgt_start_addr .req R8
pgt_num .req R7

.align 2
_loader:
	
	/* Set the svc stack, remember we need to use the loaded physical address */
	/* Not the virtual address */
	LDR SP, =__svc_stack_bottom+4096-0xF0000000

	/* __kernel_physical_end is 16 kb aligned, so this is perfect place to */
	/* put the page directory */
	MOV pgt_num, #512
	LDR placement_addr, =__kernel_physical_end

	/* This works because ARM uses PC relative addressing */
	/* Create page dir at end of kernel */
	BL _create_page_dir

	/* Create page tables */
	MOV R0, pgt_num
	BL _create_page_tables

	B _start

	B .

/* 
 * Create a page directory (1st level page table)
 */
.align 2
_create_page_dir:
	STMFD SP!, {R0, R1, R2, LR}	

	/* Allocate 16 kb for page directory */
	MOV R0, #16	
	BL _alloc
	
	/* Put that shit in pgd_addr */
	MOV pgd_addr, R0

	/* Clear the page directory, there are 4096 entries in page directory */
	MOV R1, #4096
	MOV R2, #0
	BL _memsetw

	LDMFD SP!, {R0, R1, R2, PC} 	

/* 
 * Create page tables (2nd level page tables)
 * Takes as input the # of tables to create in R0
 */
.align 2
_create_page_tables:
	STMFD SP!, {R0, R1, LR}

	/* R0 will be overwritten by _alloc */
	MOV R1, R0
	
	/* Allocate memory for page tables */
	BL _alloc

	/* Set pgt_start_addr */
	MOV	pgt_start_addr, R0

	/* There are 256 entries (words) in each page table */
	/* Clear it all */
	LSL R1, R1, #8
	MOV R2, #0
	BL _memsetw

	LDMFD SP!, {R0, R1, PC}

/* 
 * Sets a range of words to the specified value
 * Takes as input the starting address in R0, number of words in R1
 * and value to set in R2
 */
.align 2
_memsetw:
	STMFD SP!, {R0, R1, R2, LR}

_memsetw_loop:
	SUBS R1, R1, #1
	BMI _memsetw_done

	/* Store word in [R0] and increment R0 to next word */
	STR R2, [R0], #4

	B _memsetw_loop

_memsetw_done:
	LDMFD SP!, {R0, R1, R2, PC}

/* 
 * Simple allocation using the placement address (assumes this value is
 * stored in placement_addr)  as pointer to free block
 * Takes as input size of block (in kb) to allocate in R0
 * Returns the address of the beginning of the block
 */
.align 2
_alloc:
	STMFD SP!, {LR}

	/* Convert kb to bytes */
	LSL R0, R0, #10

	/* Increment placement address by block size */
	ADD placement_addr, placement_addr, R0

	/* Store starting address of block in R0 */
	SUB R0, placement_addr, R0

	LDMFD SP!, {PC}


