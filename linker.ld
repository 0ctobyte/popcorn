ENTRY(_start)
SECTIONS
{
	. = 0x10000;
	__kernel_start = .;
	
	.text ALIGN(0x4) : { /* Align to 4 byte boundary */
		kernel/boot/boot.o (.text)
		*(.text)
	}

	.data ALIGN(0x4) : {
		*(.data)
	}

	.stacks ALIGN(0x4) : {
		*(.stacks)
	}

	.rodata ALIGN(0x4) : {
		*(.rodata)
	}
	
	.bss ALIGN(0x4) : {
		*(.bss)
		*(.COMMON)
	}
	
	__kernel_end = ADDR(.bss) + SIZEOF(.bss);
}

