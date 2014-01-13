ENTRY(_start)
SECTIONS
{
	. = 0x10000;
	__kernel_start = .;
	
	.text ALIGN(0x4) : { /* Align to 4 byte boundary */
		kernel/boot/boot.o (.text)
		*(.text)
	}

	.stacks ALIGN(0x4) : {
		*(.stacks)
	}

	.bss ALIGN(0x4) : {
		*(.bss)
		*(.COMMON)
	}

	.rodata ALIGN(0x4) : {
		*(.rodata)
	}

	.data ALIGN(0x4) : {
		*(.data)
	}

	__kernel_end = __kernel_start + SIZEOF(.text) + SIZEOF(.stacks) + SIZEOF(.bss) + SIZEOF(.rodata) + SIZEOF(.data);
}

