AS := arm-none-eabi-as
CC := arm-none-eabi-gcc
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

LSCRIPT := linker.ld

BASEFLAGS := -g -pedantic -pedantic-errors -nostdlib
BASEFLAGS += -nostartfiles -ffreestanding -nodefaultlibs
BASEFLAGS += -fno-builtin -mcpu=cortex-a8

WARNFLAGS   := -Wall -Wextra -Wshadow -Wcast-align -Wwrite-strings
WARNFLAGS   += -Wredundant-decls -Winline
WARNFLAGS   += -Wno-attributes -Wno-deprecated-declarations
WARNFLAGS   += -Wno-div-by-zero -Wno-endif-labels -Wfloat-equal
WARNFLAGS   += -Wformat=2 -Wno-format-extra-args -Winit-self
WARNFLAGS   += -Winvalid-pch -Wmissing-format-attribute
WARNFLAGS   += -Wmissing-include-dirs -Wno-multichar
WARNFLAGS   += -Wredundant-decls -Wshadow
WARNFLAGS   += -Wno-sign-compare -Wswitch -Wsystem-headers -Wundef
WARNFLAGS   += -Wno-pragmas -Wno-unused-but-set-parameter
WARNFLAGS   += -Wno-unused-but-set-variable -Wno-unused-result
WARNFLAGS   += -Wwrite-strings -Wdisabled-optimization -Wpointer-arith
WARNFLAGS   += -Werror

CFLAGS := -std=c99 $(DEFINES) $(BASEFLAGS) $(WARNFLAGS) $(INCLUDE)
LDFLAGS := $(BASEFLAGS)
ASFLAGS := -g -gstabs

LIBS := -lgcc

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


