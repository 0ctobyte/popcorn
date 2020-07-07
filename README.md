# Popcorn: 64-Bit ARM Kernel

This is (or will be) a 64-bit micro-kernel targeting ARMv8-A based systems. The kernel is heavily inspired by the CMU Mach kernel.

Currently the kernel is able to boot and run in virtual memory mode on the following systems:

|System                       | CPU           | Memory      | Emulated? |
|---------------------------- |---------------|-------------|-----------|
|QEMU ARM Virt Platform       |ARM Cortex-A53 |Configurable |Yes (QEMU) |
|QEMU Raspberry Pi 3          |ARM Cortex-A53 |Configurable |Yes (QEMU) |

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
 - sys          (Unix compatibility headers)
- kernel        (Core kernel code)
- utils         (Miscellaneous functions & my own (hacky) implementation of parts of the C standard library)
- platform      (Board/platform dependent code)
 - virt         (code for the "virt" QEMU emulated platform)
 - raspberrypi3 (code for the raspberrypi3 QEMU emulated platform)
- tools         (Scripts to run the QEMU emulator)

## Todo
The code is still incomplete. As of now, the kernel boots and runs in virtual memory mode on an emulated ARM platform board as well as on real hardware (Raspberry Pi 3) and is able to print text over UART.

Next steps:
* Virtual memory management system and pmap
* process and threads
* Virtual file system
