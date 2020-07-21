# GDB/QEMU How To

Run the QEMU emulator:

`./run-qemu.py <kernel.img> -debug`

And then run GDB:

`aarch64-none-elf-gdb -tui -command=qemu_gdbinit`

This should bring up a GDB prompt and should already be connected to QEMU. If not, run this command in the GDB prompt:

`target remote localhost:1234`

The GDB TUI (text user interface) allows several useful layouts:
* `layout src` displays the source window
* `layout asm` displays the assembly window
* `layout split` displays the source and assembly window
* `layout reg` displays the register window together with the source or assembly window

You can also focus (make active) on a certain window (for scrolling):
* `focus cmd` focus on the command window which is always present
* `focus src` focus on the source window
* `focus asm` focus on the assembly window
* `focus reg` focus on the register window

If you want to display a different register set in the register window:
* `tui reg float` Display the floating point registers in the register window
* `tui reg system` Display the system registers in the register window
* `tui reg next` Display the next register group. You can cycle through register groups with this command
* `tui reg general` Display the general purpose registers

# OpenOCD/GDB Raspberry Pi 3 Debugging

Plug in a JTAG USB cable attached to your Raspberry Pi 3 and power it on.

Run OpenOCD in the tools directory which will automatically use the openocd.cfg script:

`sudo openocd`

And then run GDB:

`aarch64-none-elf-gdb -tui -command=openocd_gdbinit`
