ENTRY(_start)
SECTIONS
{
	. = 0x10000;
	.text : ALIGN(0x4) { /* Align to 4 byte boundary */
		__kernel_start = .;
		
		__text_start = .;
		kernel/boot/boot.o(.text*)
		*(.text*)
		__text_end = .;
		
		. = ALIGN(0x4);
		__rodata_start = .;
		*(.rodata*)
		__rodata_end = .;

		. = ALIGN(0x4);
		__data_start = .;
		*(.data*)
		__data_end = .;

		. = ALIGN(0x4);
		__bss_start = .;
		*(.bss*)
		. = ALIGN(4);
		*(.COMMON*)
		__bss_end = .;

		. = ALIGN(0x4);
		__kernel_stacks_start = .;
		*(.stacks*)
		__kernel_stacks_end = .;

		__kernel_end = .;
	}
	/* So apparently these sections don't get loaded in qemu... */
	/* We will find out why another day, for now, everything */
	/* goes in the text section */
	/*
	.rodata : ALIGN(0x1000) {
		*(.rodata*)
	}
	.data : ALIGN(0x1000) {
		*(.data*)
	}
	.bss : ALIGN(0x1000) { 
		*(.bss*)
		*(COMMON*)
	}
	.stacks : ALIGN(0x1000) {
		*(.stacks*)
	}*/
}

