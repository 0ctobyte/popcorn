AS := clang
CC := clang
LD := arm-none-eabi-ld
OBJCOPY := objcopy

# TODO: Need a more flexible build system
PLATFORM := beagleboneblack

ifeq ($(PLATFORM), beagleboneblack)
DEFINES := -DBBB
else ifeq ($(PLATFORM), realviewpb)
DEFINES := -DREALVIEWPB
else ifeq ($(PLATFORM), versatilepb)
DEFINES := -DVERSATILEPB
else
$(error Invalid platform)
endif

C_SRCS := $(wildcard boot/*.c) $(wildcard platform/$(PLATFORM)/*.c) $(wildcard platform/*.c) $(wildcard kernel/*.c) $(wildcard lib/*.c)
S_SRCS := $(wildcard boot/*.s) $(wildcard platform/$(PLATFORM)/*.s) $(wildcard platform/*.s) $(wildcard kernel/*.s) $(wildcard lib/*.s)

OBJS := $(patsubst %.s,%.o,$(S_SRCS))
OBJS += $(patsubst %.c,%.o,$(C_SRCS))

INCLUDE := -Iinclude
LSCRIPT := linker.ld

BASEFLAGS := -g -target armv7-none-eabi -mcpu=cortex-a8 -mfloat-abi=hard -mfpu=vfpv3
WARNFLAGS := -Weverything -Werror -Wno-missing-prototypes -Wno-unused-macros -Wno-bad-function-cast -Wno-sign-conversion
CFLAGS := -std=c99 -fno-builtin -ffreestanding -fomit-frame-pointer $(DEFINES) $(BASEFLAGS) $(WARNFLAGS) $(INCLUDE)
LDFLAGS := -nostdlib -nostdinc -nodefaultlibs -nostartfiles -T $(LSCRIPT)
ASFLAGS := $(BASEFLAGS) $(WARNFLAGS) 
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

