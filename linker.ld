ENTRY(_vectors_start)
SECTIONS
{
	. = 0x10000;
	__text_start = .;
	.text : {
		*(.text*)
	}
	. = ALIGN(4096); /* Align to 4kB page */
	__text_end = .;

	__rodata_start = .;
	.rodata : {
		*(.rodata*)
	}
	. = ALIGN(4096); /* Align to 4kB page */
	__rodata_end = .;

	__data_start = .;
	.data : {
		*(.data*)
	}
	. = ALIGN(4096); /* Align to 4kB page */
	__data_end = .;

	__bss_start = .;
	.bss : { 
		*(.bss*)
		*(COMMON*)
	}
	. = ALIGN(4096); /* Align to 4kB page */
	__bss_end = .;

	__kernel_stacks_start = .;
	. = . + 0x1000; /* 4kB of supervisor stack */
	supervisor_stack_top = .;
	. = ALIGN(4096); /* Align to 4kB page */
	. = . + 0x1000; /* 4kB of IRQ stack */
	irq_stack_top = .;
	. = ALIGN(4096); /* Align to 4kB page */
	__kernel_stacks_end = .;
}

