# Popcorn: 64-Bit ARM Kernel

This is (or will be) a 64-bit micro-kernel targeting ARMv8-A based systems.
The kernel is heavily inspired by the CMU Mach kernel and uses ideas from the FreeBSD project.

Currently the kernel is able to boot and run in virtual memory mode on the following systems:

|System                       |CPU            |Memory       |Emulated?  |
|---------------------------- |---------------|-------------|-----------|
|QEMU ARM Virt Platform       |ARM Cortex-A53 |Configurable |Yes (QEMU) |
|QEMU Raspberry Pi 3          |ARM Cortex-A53 |Configurable |Yes (QEMU) |
|Raspberry Pi 3               |ARM Cortex-A53 |960MB        |No         |
|Raspberry Pi 4               |ARM Cortex-A72 |960MB        |No         |

It's still a work in progress and is far from complete.

## Build
You will need to have the aarch64-none-elf-gcc toolchain to build the code.
Refer to GCC documention to build a gcc targetting aarch64-none-elf.
For MacOS, you can install the toolchain from [here](https://github.com/SergioBenitez/homebrew-osxct) using homebrew.

To compile:
`mkdir build && cd build && cmake ../ && make`

This should produce `kernel.bin` which is a flat binary image of the kernel.

To run on the QEMU emulator simply run the `run-qemu.py` script in the `tools` directory with the path to kernel.bin.
See the `README.md` in the tools folder for usage info.
QEMU must already be installed for the script to work.

To debug the kernel with GDB:

1. Run the `run-qemu.py` script with the `-debug` option
2. Run `aarch64-none-elf-gdb` in the terminal
3. If GDB does not automatically connect to the QEMU debugger. Type `target remote localhost:1234` in the GDB prompt to connect to the QEMU debugger

## Code Structure
Here is the directory structure to help with navigation of the code.

|Path                |Description                                                                                                                              |
|--------------------|-----------------------------------------------------------------------------------------------------------------------------------------|
|kernel/             |Main kernel sources. This contains the bulk of the code                                                                                  |
|kernel/arch         |Architecture specific code. Obviously right now this is all just AARCH64 code                                                            |
|kernel/proc         |Process, threads and scheduling code                                                                                                     |
|kernel/vfs          |This will be the virtual file system code. It's based off the FreeBSD/SunOS VFS model                                                    |
|kernel/vm           |Architecture independent virtual memory management code. This code is modelled after the CMU Mach 4 virtual memory management sub-system |
|utils/              |Various utility functions like vsprintf and implementations of the std C lib string functions                                            |
|devices/            |Contains the minimal set of supported devices that the kernel needs to run. For now, this means an interrupt controller and serial device|
|platform/           |Platform specific code. Right now only QEMU-virt and RPi3 platforms are supported                                                        |
