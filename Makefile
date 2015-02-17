AS := clang
CC := clang
LD := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy

# TODO: Need a more flexible build system
PLATFORM=realview-pb
DEFINES := -DREALVIEW_PB

C_SRCS := $(wildcard boot/*.c) $(wildcard platform/$(PLATFORM)/*.c) $(wildcard platform/*.c) $(wildcard kernel/*.c) $(wildcard lib/*.c)
S_SRCS := $(wildcard boot/*.s) $(wildcard platform/$(PLATFORM)/*.s) $(wildcard platform/*.s) $(wildcard kernel/*.s) $(wildcard lib/*.s)

OBJS := $(patsubst %.s,%.o,$(S_SRCS))
OBJS += $(patsubst %.c,%.o,$(C_SRCS))

INCLUDE := -Iinclude
LIBS := -lgcc
LSCRIPT := linker.ld

BASEFLAGS := -g -target armv7-none-eabi -mcpu=cortex-a8 -mfloat-abi=hard -mfpu=vfpv3
WARNFLAGS := -Weverything -Werror -Wno-missing-prototypes -Wno-unused-macros -Wno-bad-function-cast -Wno-sign-conversion
CFLAGS := -std=c99 -fno-builtin -ffreestanding -fomit-frame-pointer $(DEFINES) $(BASEFLAGS) $(WARNFLAGS) $(INCLUDE)
LDFLAGS := -nostdlib -nostdinc -nodefaultlibs -nostartfiles
ASFLAGS := $(BASEFLAGS) $(WARNFLAGS) 

all : kernel.img

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -T $(LSCRIPT) -o $@

kernel.img: kernel.elf
	$(OBJCOPY) kernel.elf -O binary kernel.img

clean:
	$(RM) -f $(OBJS) kernel.elf kernel.img

dist-clean: clean
	$(RM) -f *.d

%.o: %.s Makefile
	$(AS) $(ASFLAGS) -c $< -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@


