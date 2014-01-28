# Project Name
PROJECT = arm-bmw-selftest
# Source files
SOURCES_TESTS = tests/uart.c tests/spi.c tests/spi_flash.c tests/i2c.c
SOURCES = lpc11xx/system_LPC11xx.c startup.c tick.c uart.c debug.c spi.c sf.c queue.c i2c.c $(SOURCES_TESTS) main.c
# Linker script
LINKER_SCRIPT = lpc1114.dld

#########################################################################

OBJDIR = obj
OBJECTS = $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))

#########################################################################

OPT = -Os
DEBUG = -g
INCLUDES = -Icore/ -I.
GIT_VERSION = $(shell git describe --abbrev --always --dirty)

#########################################################################

# Compiler Options
CFLAGS = -DGIT_VERSION=\"$(GIT_VERSION)\"
CFLAGS += -fno-common -mcpu=cortex-m0 -mthumb
CFLAGS += $(OPT) $(DEBUG) $(INCLUDES)
CFLAGS += -Wall -Wextra
CFLAGS += -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused
# Linker options
LDFLAGS = -mcpu=cortex-m0 -mthumb $(OPT) -nostartfiles -Wl,-Map=$(PROJECT).map -T$(LINKER_SCRIPT)
# Assembler options
ASFLAGS = -ahls -mcpu=cortex-m0 -mthumb

# Compiler/Assembler/Linker Paths
CROSS = arm-none-eabi-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size
REMOVE = rm -f

#########################################################################

all: clean $(PROJECT).hex $(PROJECT).bin

flash: $(PROJECT).elf
	openocd -s openocd -f openocd/flash.cfg

debug: $(PROJECT).elf
	openocd -s openocd -f openocd/debug.cfg

gdb: $(PROJECT).elf
	$(CROSS)gdb

flashisp: $(PROJECT).hex
	lpc21isp -verify $(PROJECT).hex /dev/ttyUSB1 115200 12000000

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -R .stack -R .bss -O binary -S $(PROJECT).elf $(PROJECT).bin

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -R .stack -R .bss -O ihex $(PROJECT).elf $(PROJECT).hex

$(PROJECT).elf: $(OBJECTS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(PROJECT).elf

stats: $(PROJECT).elf
	$(OBJDUMP) -th $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

clean:
	$(REMOVE) -r $(OBJDIR)
	$(REMOVE) $(PROJECT).elf
	$(REMOVE) $(PROJECT).hex
	$(REMOVE) $(PROJECT).bin
	$(REMOVE) $(PROJECT).map

#########################################################################

$(OBJECTS): | $(OBJDIR)

$(OBJDIR):
	mkdir $(OBJDIR)
	mkdir $(OBJDIR)/lpc11xx
	mkdir $(OBJDIR)/core
	mkdir $(OBJDIR)/tests

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

