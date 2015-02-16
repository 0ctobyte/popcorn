ENTRY(_loader)
SECTIONS
{
	. = 0xF0010000;
	__kernel_virtual_start = .;
	__kernel_physical_start = . - 0xF0000000;
	
	/* Align to page (4 kb) boundary */
	__text_virtual_start = ALIGN(.);
	__text_physical_start = ALIGN(.) - 0xF0000000;
	.text ALIGN(0x1000) : AT(ADDR(.text) - 0xF0000000) {
		*(.text)
	}
	__text_virtual_end = .;
	__text_physical_end = . - 0xF0000000;

	__data_virtual_start = ALIGN(0x1000);
	__data_physical_start = ALIGN(0x1000) - 0xF0000000;
	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xF0000000) {
		*(.rodata)
	}
		
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xF0000000) {
		*(.data)
	}

	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xF0000000) {
		*(.bss)
		*(.COMMON)
	}

	__data_virtual_end = .;
	__data_physical_end = . - 0xF0000000;
	
	__kernel_virtual_end = .;
	__kernel_physical_end = . - 0xF0000000;

	/* Align to 16 kb boundary */
	__pgd_virtual_start = ALIGN(0x4000);
	__pgd_physical_start = ALIGN(0x4000) - 0xF0000000;

	/* Page tables will always be created directly after the page directory */
	__pgt_virtual_start = ALIGN(0x4000) + 0x4000;
	__pgt_physical_start = ALIGN(0x4000) + 0x4000 - 0xF0000000;

	/* Number of page tables needed to map the kernel virtual memory space */
	__pgt_num = 0x100;
}

