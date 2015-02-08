# 32-Bit ARM Kernel

This is (or will be) a 32-bit kernel for the ARMv7-A architecture

Currently the kernel is able to load and run on an emulated (using QEMU) 
versatile-pb board with an ARM Cortex-A8 CPU and 128M of RAM.

## Build
Need to have the arm-none-eabi-gcc toolchain to compile from source.
[This](http://blog.y3xz.com/blog/2012/10/07/setting-up-an-arm-eabi-toolchain-on-mac-os-x) site is a good place to start to install on Mac OS.

To compile:
`make`

This should produce `kernel.img` which is a binary image of the kernel.

To run on the QEMU emulator simply run the `sim.sh` script in the `tools` directory.

To debug the kernel with GDB:
1. Run the `gdb.sh` script
2. Run `arm-none-eabi-gdb` in the terminal
3. Type `target remote localhost:1234` in the GDB prompt to connect to the QEMU debugger

