# 32-Bit ARM Kernel

This is (or will be) a 32-bit kernel for the ARMv7-A architecture

Currently the kernel is able to boot and run in virtual memory mode on an emulated (using QEMU) 
Versatile platform board and RealView platform board with an ARM Cortex-A8 CPU and 128M of RAM.

## Build
Need to have the arm-none-eabi-gcc toolchain for the ld (linker) and binutils for the objcopy tool. Clang is used for compilation.
[This](http://blog.y3xz.com/blog/2012/10/07/setting-up-an-arm-eabi-toolchain-on-mac-os-x) site is a good place to start to install on Mac OS.

To compile:
`make`

This should produce `kernel.img` which is a binary image of the kernel.

To run on the QEMU emulator simply run the `emulator` script in the `tools` directory (See the `README.md` in the tools folder for usage info).
QEMU must already be installed for the script to work.

To debug the kernel with GDB:

1. Run the `emulator` script
2. Run `arm-none-eabi-gdb` in the terminal
3. Type `target remote localhost:1234` in the GDB prompt to connect to the QEMU debugger

## Code Structure
Here is the directory structure to help with navigation of the code.

- boot          (Boot/startup code)
- include       (Headers)
 - kernel       (Kernel headers)
 - lib          (lib headers)
 - platform     (Platform headers)
 - sys          (Unix compatibility headers)
- kernel        (Core kernel code)
- lib           (Miscellaneous functions & my own (hacky) implementation of parts of the C standard library)
- platform      (Board/platform dependent code)
 - versatile-pb (code for the ARM Versatile platform board)
 - realview-pb  (code for the ARM RealView platform board)
- tools         (Scripts to run the QEMU emulator)

Here is a short description of what each code file does:

##### boot
The boot (or rather startup) code are the first instructions executed by the CPU on bootup. 

The boot code consists of `_loader.s` which is located at the beginning of the binary image (thus, executed first), `atags.s` and `boot.s`.
On startup, the `_loader` function in `_loader.s` is executed. This function jumps to code located in atags.s to parse any ATAGs (ARM tags) in memory to 
retrieve useful system info such as memory size and start address (DeviceTrees are a better solution, but this will do for now).

`_loader` then begins setting up the page directory/page tables by mapping the kernel's virtual address space (starting at 0xF0000000). Once this is done, `_loader` then
enables the MMU and jumps to the `_start` function located in `boot.s`. The `_start` simply sets up the stack pointers for each of the processor modes, enables the VFPU
(vector floating point unit) and branches to `kmain` (located in `kmain.c`).

##### kernel
The kernel code consists of several different modules. 

`kmain` is the main entry point of the kernel after the boot up code. The `pmm.c` module contains the 
physical page frame memory allocator. `pmap.c` contains the code to manage virtual address mappings in the page tables. `spinlock.c` contains a simple spinlock 
implementation. `_atomic.s` contains the instructions for atomic accesses to memory. `_barrier.s` contains memory/data/instruction barrier instructions. `_evt.s` 
contains code to set up the ARM exception vectors and register handlers for each of the ARM exceptions. `_interrupts.s` contains the instructions to enable or
disable interrupts. `kstdio.c` for an implementation of the printf (`kprintf`) function for use by the kernel (since no standard libraries can be used).
The `panic.c` module contains the `panic` function which prints an error message and halts the cpu.

##### lib
The lib code consists of useful helper functions and implementations of some of the standard C library functions.

##### Todo
The code is still incomplete. As of now, the kernel boots and runs in virtual memory mode on an emulated ARM platform board and is able to print text over UART.

Next steps:
* Need a kernel heap/slab allocator to allocate kernel structures (pmaps, pgd, pgts etc.)
* Managing platform specific IO devices and memory mappings 
* Initialization of platform specific devices 
* Better build system
* Debugging output on aborts/exceptions (register dump, stack trace)
* Enabling and modifying functionality of caches, TLBs, and CPU (branch prediction) through the control registers

