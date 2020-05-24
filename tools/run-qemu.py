#!/usr/bin/env python3

import subprocess
import argparse

parser = argparse.ArgumentParser(description="QEMU run tool", formatter_class=argparse.RawTextHelpFormatter)
parser.add_argument("bin", help="Kernel binary image to load")
parser.add_argument("-debug", help="Enable QEMU GDB mode and halt execution after reset", default=False, action="store_true")
parser.add_argument("-memsize", help="RAM size", default="512M")
parser.add_argument("-cpu", help='''
                                Type of CPU. Select from:
                                - cortex-a53
                                - cortex-a57
                                - cortex-a72
                                ''', default="cortex-a53")
parser.add_argument("-cores", help="Number of cores to emulate", default=1)
parser.add_argument("-graphics", help="Enable graphical mode (i.e. display)", default=False, action="store_true")
parser.add_argument("-dtb", help="Use the given DeviceTree blob file", default="")

args = parser.parse_args()

qemu = ["qemu-system-aarch64", "-M", "virt"]
qemu += ["-m", args.memsize]
qemu += ["-cpu", args.cpu]
qemu += ["-smp", "cores="+ str(args.cores)]
if not args.graphics:
    qemu += ["-nographic"]
if args.debug:
    qemu += ["-s", "-S"]
if args.dtb != "":
    qemu += ["-dtb", args.dtb]
qemu += ["-kernel", args.bin]

print("Running QEMU ARM Virt platform with command-line: " + ' '.join(qemu))
print("CPU: " + args.cpu + " CORES: " + str(args.cores))
print("RAM SIZE: " + args.memsize)
if args.graphics:
    print("Running with graphics enabled")
if args.debug:
    print("Running with GDB mode enabled. Execution halted by QEMU. You will need to use GDB to continue running the code.")

subprocess.call(qemu)
