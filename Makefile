AS := aarch64-none-elf-gcc
CC := aarch64-none-elf-gcc
LD := aarch64-none-elf-ld
OBJCOPY := aarch64-none-elf-objcopy

# TODO: Need a more flexible build system
PLATFORM := virt

ifeq ($(PLATFORM), virt)
DEFINES := -DVIRT
else
$(error Invalid platform)
endif

C_SRCS := $(wildcard boot/*.c) $(wildcard platform/$(PLATFORM)/*.c) $(wildcard platform/*.c) $(wildcard kernel/*.c) $(wildcard lib/*.c)
S_SRCS := $(wildcard boot/*.s) $(wildcard platform/$(PLATFORM)/*.s) $(wildcard platform/*.s) $(wildcard kernel/*.s) $(wildcard lib/*.s)

OBJS := $(patsubst %.s,%.o,$(S_SRCS))
OBJS += $(patsubst %.c,%.o,$(C_SRCS))

INCLUDE := -Iinclude
LSCRIPT := linker.ld

BASEFLAGS := -g -march=armv8-a
WARNFLAGS := -Werror
CFLAGS := -std=c99 -fno-builtin -ffreestanding -fomit-frame-pointer $(DEFINES) $(BASEFLAGS) $(WARNFLAGS) $(INCLUDE)
LDFLAGS := -nostdlib -nostdinc -nodefaultlibs -nostartfiles -T $(LSCRIPT)
ASFLAGS := $(DEFINES) $(BASEFLAGS) $(WARNFLAGS) -x assembler-with-cpp
OCFLAGS := --target elf32-littlearm --set-section-flags .bss=contents,alloc,load -O binary

kernel.img: kernel.elf
	$(OBJCOPY) $(OCFLAGS) kernel.elf kernel.img

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.s Makefile
	$(AS) $(ASFLAGS) -c $< -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -f $(OBJS) kernel.elf kernel.img

