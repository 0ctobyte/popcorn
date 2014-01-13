ENTRY(_start)
SECTIONS
{
	. = 0x10000;
	__kernel_start = .;
	
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

	.text ALIGN(0x4) : { /* Align to 4 byte boundary */
		kernel/boot/boot.o (.text)
		*(.text)
	}

	/* __kernel_end will ALWAYS be 0x0 UNLESS I assign it exactly this way */
	/* WTF is magic? I don't understand how this piece of shit linker behaves no matter */
	/* how many fucking manuals I read. SHiiiiieeettttt */
	/* Anyway this should give me the end of the kernel image...I think... */
	__kernel_end = ADDR(.text) + SIZEOF(.text);
}

