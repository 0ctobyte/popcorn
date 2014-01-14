ENTRY(_start)
SECTIONS
{
	. = 0x10000;
	__kernel_start = .;
	
	.text ALIGN(0x4) : { /* Align to 4 byte boundary */
		*(.text)
	}

	.data ALIGN(0x4) : {
		*(.data)
	}

	.rodata ALIGN(0x4) : {
		*(.rodata)
	}

	.bss ALIGN(0x4) : {
		*(.bss)
		*(.COMMON)
	}
	
	__kernel_end = .;
}

