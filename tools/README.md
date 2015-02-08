# How To Debug

1. `gdb.sh`
2. `arm-none-eabi-gdb -tui`

This should bring up a GDB prompt. To connect to the QEMU debugger, enter into the GDB prompt:

`target remote localhost:1234`

