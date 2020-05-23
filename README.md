# 64-Bit ARM Kernel

This is (or will be) a 64-bit micro-kernel targeting ARMv8-A based systems

Currently the kernel is able to boot and run in virtual memory mode on the following systems:

|System                       | CPU           | Memory      | Emulated? |
|---------------------------- |---------------|-------------|-----------|
|QEMU ARM Virt Platform       |ARM Cortex-A53 |Configurable |Yes (QEMU) |

## Build
You will need to have the aarch64-none-elf-gcc toolchain to build the code. Refer to GCC documention to build a gcc targetting aarch64-none-elf.
For MacOS, you can install the toolchain from [here](https://github.com/SergioBenitez/homebrew-osxct) using homebrew:

To compile:
`mkdir build && cd build && cmake ../ && make`

This should produce `kernel.img` which is a flat binary image of the kernel.

To run on the QEMU emulator simply run the `run-qemu.py` script in the `tools` directory with the path to kernel.img (See the `README.md` in the tools folder for usage info).
QEMU must already be installed for the script to work.

To debug the kernel with GDB:

1. Run the `run-qemu.py` script with the `-debug` option
2. Run `aarch64-none-elf-gdb` in the terminal
3. If GDB does not automatically connect to the QEMU debugger. Type `target remote localhost:1234` in the GDB prompt to connect to the QEMU debugger

## Code Structure
Here is the directory structure to help with navigation of the code.

- include       (Headers)
 - kernel       (Kernel headers)
 - lib          (lib headers)
 - platform     (Platform headers)
 - sys          (Unix compatibility headers)
- kernel        (Core kernel code)
- lib           (Miscellaneous functions & my own (hacky) implementation of parts of the C standard library)
- platform      (Board/platform dependent code)
 - virt         (code for the "virt" QEMU emulated platform)
- tools         (Scripts to run the QEMU emulator)

Here is a short description of what each code file does:

##### boot

##### kernel
The kernel code consists of several different modules.

The boot (or rather startup) code are the first instructions executed by the CPU on bootup.

The boot code is found in `__start.s` which is located at the beginning of the binary image (thus, executed first). It will relocate the kernel to a 64KB aligned page if necessary,
setup the stack and copy the FDT (flattened device tree blob) somewhere safe and then jump to kmain.

`kmain` is the main entry point of the kernel after the boot up code. The `pmm.c` module contains the physical page frame memory allocator. `pmap.c` contains the code to manage virtual
address mappings in the page tables. `spinlock.c` contains a simple spinlock implementation. `_atomic.s` contains the instructions for atomic accesses to memory.
`_barrier.s` contains memory/data/instruction barrier instructions. `_exceptions.s` contains code to set up the AARCH64 exception vectors and the handler to save/restore state from
exception entry/return. `_interrupts.s` contains the instructions to enable or disable interrupts. `kstdio.c` for an implementation of the printf (`kprintf`) function for use by the
kernel (since no standard libraries can be used). The `panic.c` module contains the `panic` function which prints an error message and halts the cpu. The `devicetree` module is
used to search the devicetree for important system information at boot; specifically the memory base address and size and the UART devices so the kernel can use kprintf.

##### lib
The lib code consists of useful helper functions and implementations of some of the standard C library functions.

## Todo
The code is still incomplete. As of now, the kernel boots and runs in virtual memory mode on an emulated ARM platform board as well as on real hardware (Beaglebone Black) and is able to print text over UART.

Next steps:
* Need a slab allocator to allocate kernel structures (pmaps, pgd, pgts etc.)
* Managing platform specific IO devices and memory mappings
* Initialization of platform specific devices
* Better build system
* Debugging output on aborts/exceptions (register dump, stack trace)
* Enabling and modifying functionality of caches, TLBs, and CPU (branch prediction) through the control registers

