## Project Name
PROJECT = arm-bmw-selftest
APP_SRCS = selftest/main.c selftest/cli_programs.c selftest/rpc_handlers.c
APP_SRCS += selftest/tests/test_uart.c selftest/tests/test_spi.c selftest/tests/test_spi_flash.c selftest/tests/test_i2c.c selftest/tests/test_mcp23008.c selftest/tests/test_bmw_ui.c

## BMW Lib Source files
# system sources
BMW_SRCS = system/lpc11xx/system_LPC11xx.c system/startup.c system/tick.c
# peripheral io sources
BMW_SRCS += io/uart.c io/spi.c io/i2c.c io/i2c_reg.c io/queue.c
# driver sources
BMW_SRCS += driver/sf.c driver/mcp23008.c
# high level sources
BMW_SRCS += debug.c bmw_ui.c ucli.c urpc.c

# Linker script
LINKER_SCRIPT = lpc1114.dld

#########################################################################

SRCDIR = src
APPDIR = app
OBJDIR = obj

BMW_OBJECTS = $(patsubst %.c,$(OBJDIR)/$(SRCDIR)/%.o,$(BMW_SRCS))
APP_OBJECTS = $(patsubst %.c,$(OBJDIR)/$(APPDIR)/%.o,$(APP_SRCS))
BMW_LIB = libbmw.a

#########################################################################

OPT = -Os
DEBUG =
INCLUDES = -Isrc/system/cmsis/ -Isrc/
GIT_VERSION = $(shell git describe --abbrev --always --dirty)

#########################################################################

# Compiler Options
CFLAGS = -DGIT_VERSION=\"$(GIT_VERSION)\"
CFLAGS += -fno-common -mcpu=cortex-m0 -mthumb
CFLAGS += $(OPT) $(DEBUG) $(INCLUDES)
CFLAGS += -Wall -Wextra
CFLAGS += -Wcast-align -Wcast-qual -Wimplicit -Wpointer-arith -Wswitch -Wredundant-decls -Wreturn-type -Wshadow -Wunused -Wno-unused-parameter

# Linker options
LDFLAGS = -mcpu=cortex-m0 -mthumb $(OPT) -nostartfiles -Wl,-Map=$(OBJDIR)/$(PROJECT).map -T$(LINKER_SCRIPT)

# Assembler options
ASFLAGS = -ahls -mcpu=cortex-m0 -mthumb

# Compiler/Assembler/Linker Paths
CROSS = arm-none-eabi-
AR = $(CROSS)ar
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

$(PROJECT).elf: $(BMW_LIB) $(APP_OBJECTS) $(LINKER_SCRIPT)
	$(CC) $(LDFLAGS) $(APP_OBJECTS) -o $(PROJECT).elf $(BMW_LIB)

stats: $(PROJECT).elf
	$(OBJDUMP) -th $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

clean:
	$(REMOVE) -r $(OBJDIR)
	$(REMOVE) $(BMW_LIB)
	$(REMOVE) $(PROJECT).elf
	$(REMOVE) $(PROJECT).hex
	$(REMOVE) $(PROJECT).bin

#########################################################################

$(BMW_LIB): $(BMW_OBJECTS)
	$(AR) rcs $@ $(BMW_OBJECTS)

$(OBJDIR)/%.o : %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

