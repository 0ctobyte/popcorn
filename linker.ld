ENTRY(_start)
SECTIONS
{
    __virtual_base = 0xFFFF000000000000;
    . = __virtual_base;
    __kernel_virtual_start = .;
    __kernel_physical_start = . - __virtual_base;

    /* Align to page (4 kb) boundary */
    .text ALIGN(0x1000) : AT(ADDR(.text) - __virtual_base) {
        *(.text)
    }
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - __virtual_base) {
        *(.rodata)
    }

    .data ALIGN(0x1000) : AT(ADDR(.data) - __virtual_base) {
        *(.data)
    }

    __kernel_physical_bss_start = . - __virtual_base;
    .bss ALIGN(0x1000) : AT(ADDR(.bss) - __virtual_base) {
        *(.bss)
        *(.COMMON)
    }
    __kernel_physical_bss_end = . - __virtual_base;

    __kernel_virtual_end = .;
    __kernel_physical_end = . - __virtual_base;
}

