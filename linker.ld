ENTRY(_loader)
SECTIONS
{
	. = 0xF0010000;
	__kernel_virtual_start = .;
	__kernel_physical_start = . - 0xF0000000;
	
	/* Align to page (4 kb) boundary */
	__text_virtual_start = .;
	__text_physical_start = . - 0xF0000000;
	.text ALIGN(0x1000) : AT(ADDR(.text) - 0xF0000000) {
		*(.text)
	}
	__text_virtual_end = ALIGN(0x1000);
	__text_physical_end = ALIGN(0x1000) - 0xF0000000;

	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xF0000000) {
		*(.rodata)
	}
		
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xF0000000) {
		*(.bss)
		*(.COMMON)
	}
	
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xF0000000) {
		*(.data)
	}

	/* Align to 16 kb boundary */
	. = ALIGN(0x4000);
	__kernel_virtual_end = .;
	__kernel_physical_end = . - 0xF0000000;
}

