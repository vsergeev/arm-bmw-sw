# Project Name
PROJECT = blink
# List of the objects files to be made
OBJECTS = lpc11xx/system_LPC11xx.o startup.o main.o
# Linker script
LINKER_SCRIPT = lpc1114.dld

OPT = -Os
DEBUG =
INCLUDES = -Icore/ -Ilpc11xx/

# Compiler Options
CFLAGS = -fno-common -mcpu=cortex-m0 -mthumb
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

all: $(PROJECT).hex $(PROJECT).bin

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
	$(REMOVE) $(OBJECTS)
	$(REMOVE) $(PROJECT).elf
	$(REMOVE) $(PROJECT).hex
	$(REMOVE) $(PROJECT).bin
	$(REMOVE) $(PROJECT).map

#########################################################################

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

#########################################################################

