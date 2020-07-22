# GDB/QEMU How To

Run the QEMU emulator:

`./run-qemu.py <kernel.bin> -debug`

And then run GDB:

`aarch64-none-elf-gdb -tui -command=qemu_gdbinit`

This should bring up a GDB prompt and should already be connected to QEMU. If not, run this command in the GDB prompt:

`target remote localhost:1234`

You can find more information on how to use the GDB text user interface [here](https://sourceware.org/gdb/onlinedocs/gdb/TUI.html).

# OpenOCD/GDB Raspberry Pi 3 Debugging

Plug in a JTAG USB cable attached to your Raspberry Pi 3 and power it on.
You can find information on how to setup JTAG with a Raspberry Pi 3 [here](https://www.suse.com/c/debugging-raspberry-pi-3-with-jtag/).
JTAG GPIO pinout (i.e. ALT4) diagram for the Raspberry Pi 3 can be found [here](https://pinout.xyz/pinout/jtag#).

If you're using the [c232hm FTDI cable](https://www.ftdichip.com/Products/Cables/USBMPSSE.htm) the cable mapping is as follows:

|GPIO# |JTAG Signal |Wire Colour |
|------|------------|------------|
|BCM 22|TRST        |grey        |
|BCM 23|RTCK        |blue        |
|BCM 24|TDO         |green       |
|BCM 25|TCK         |orange      |
|BCM 26|TDI         |yellow      |
|BCM 27|TMS         |brown       |
|3v3   |LED         |white       |

Run OpenOCD in the tools directory:

`sudo openocd -f rpi3.cfg`

For Raspberry Pi 4:

`sudo openocd -f rpi4.cfg`

And then run GDB:

`aarch64-none-elf-gdb -tui -command=openocd_gdbinit`
