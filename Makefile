###############################################################################
#
# Makefile for C-Quencer
# Author: centrevillage
#
# For the cross-platform builds, the necessary compilers ( AVR) must either
# be in the path, or the installation directories can be specified using
#  AVR_TOOLKIT_ROOT
#
###############################################################################

# original src: https://gist.github.com/entrity/5424505

SRC_DIR=src/
BUILD_DIR=./build/
TARGET=$(BUILD_DIR)c_quencer
INCLUDES = 
# Optimization level, can be [0, 1, 2, 3, s]. 0 turns off optimization.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)
# Output format. (can be srec, ihex, binary)
AVR_OPTIMIZE=s
EXTRAINCDIRS = 
C_STD = -std=gnu99

### These macros pertain to hardware settings
ifndef MCU
MCU = atmega328p
endif
ifndef AVRDUDE_PROGRAMMER
AVRDUDE_PROGRAMMER = avrisp
endif
ifndef AVRDUDE_PORT
AVRDUDE_PORT = com4
endif
ifndef BAUDRATE
BAUDRATE = 19200
endif
ifndef F_CPU
F_CPU = 16000000
endif
DEFINES = -DF_CPU=$(F_CPU)UL -D__PROG_TYPES_COMPAT__

ifdef AVR_TOOLKIT_ROOT
BINPATH=$(AVR_TOOLKIT_ROOT)/bin/
else
BINPATH=""
endif

ifdef AVR_TOOLKIT_ROOT
AVRDUDE_CONF=-C $(AVR_TOOLKIT_ROOT)/etc/avrdude.conf
else
AVRDUDE_CONF=
endif

CC      =$(addprefix $(BINPATH),avr-gcc)
CXX     =$(addprefix $(BINPATH),avr-c++)
LD      =$(addprefix $(BINPATH),avr-ld)
CP      =$(addprefix $(BINPATH),avr-objcopy)
OD      =$(addprefix $(BINPATH),avr-objdump)
AS      =$(addprefix $(BINPATH),avr-as)
SIZE    =$(addprefix $(BINPATH),avr-size)
AVRDUDE =$(addprefix $(BINPATH),avrdude)
NM      =$(addprefix $(BINPATH),avr-nm)
REMOVE  = rm -f

SRC=$(wildcard $(SRC_DIR)*.c)
FORMAT = ihex

# List Assembler source files here.
ASRC = 

CFLAGS = -Wall -Wextra -mmcu=$(MCU) $(C_STD) -MD -MP $(DEFINES) $(INCLUDES) -O$(AVR_OPTIMIZE) $(patsubst %,-I%,$(EXTRAINCDIRS))
LDFLAGS = -Wl,-Map=$(MAP) -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,-gc-sections -mmcu=$(MCU)

# Optional assembler flags.
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -ahlms:    create listing
#  -gstabs:   have the assembler create line number information; note that
#             for use in COFF files, additional information about filenames
#             and function names needs to be present in the assembler source
#             files -- see avr-libc docs [FIXME: not yet described there]
ASFLAGS = -Wa,-adhlns=$(patsubst %.cpp,$(BUILD_DIR)%.lst,$(notdir $<)),-gstabs 

# Optional linker flags.
#  -Wl,...:   tell GCC to pass this to linker.
#  -Map:      create map file
#  --cref:    add cross reference to  map file
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref

# Programming support using avrdude. Settings and variables.
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
#AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep

AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) $(AVRDUDE_CONF)

HEXSIZE = $(SIZE) --target=$(FORMAT) $(TARGET).hex
ELFSIZE = $(SIZE) -A $(TARGET).elf

# Define Messages
# English
MSG_ERRORS_NONE = Errors: none
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before: 
MSG_SIZE_AFTER = Size after:
MSG_COFF = Converting to AVR COFF:
MSG_EXTENDED_COFF = Converting to AVR Extended COFF:
MSG_FLASH = Creating load file for Flash:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling:
MSG_CLEANING = Cleaning project:

# Define all object files.
OBJ = $(patsubst %.c,$(BUILD_DIR)%.o,$(notdir $(SRC))) $(patsubst %.S,$(BUILD_DIR)%.o,$(notdir $(ASRC))) $(patsubst %.cpp,$(BUILD_DIR)%.o,$(notdir $(PSRC)))

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS 		= -mmcu=$(MCU) -I. $(CFLAGS) -Wa,-adhlns=$(patsubst %.c,$(BUILD_DIR)%.lst,$(notdir $<))
ALL_CXXFLAGS 	= -mmcu=$(MCU) -I. $(CFLAGS) -Wa,-adhlns=$(patsubst %.cpp,$(BUILD_DIR)%.lst,$(notdir $<))
ALL_ASFLAGS 	= -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)


# Default target: make program!
all: begin gccversion sizebefore \
	$(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).lss $(TARGET).sym \
	sizeafter finished end

# Eye candy.
# AVR Studio 3.x does not check make's exit code but relies on
# the following magic strings to be generated by the compile job.
begin:
	@echo
	@echo obj = $(OBJ)
	@echo $(MSG_BEGIN)

finished:
	@echo $(MSG_ERRORS_NONE)

end:
	@echo $(MSG_END)
	@echo

# Display size of file.
sizebefore:
	@if [ -f $(TARGET).elf ]; then echo; echo $(MSG_SIZE_BEFORE); $(ELFSIZE); echo; fi

sizeafter:
	@if [ -f $(TARGET).elf ]; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); echo; fi

# Display compiler version information.
gccversion : 
	@$(CC) --version
	@echo 
	@echo $(OBJ)

# Convert ELF to COFF for use in debugging / simulating in
# AVR Studio or VMLAB.
COFFCONVERT=$(CP) --debugging \
	--change-section-address .data-0x800000 \
	--change-section-address .bss-0x800000 \
	--change-section-address .noinit-0x800000 \
	--change-section-address .eeprom-0x810000 

coff: $(TARGET).elf
	@echo
	@echo $(MSG_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-avr $< $(TARGET).cof

extcoff: $(TARGET).elf
	@echo
	@echo $(MSG_EXTENDED_COFF) $(TARGET).cof
	$(COFFCONVERT) -O coff-ext-avr $< $(TARGET).cof

# Program the device.  
program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) -b $(BAUDRATE) $(AVRDUDE_WRITE_EEPROM) -v

# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(CP) -O $(FORMAT) -R .eeprom $< $@

%.eep: %.elf
	@echo
	@echo $(MSG_EEPROM) $@
	-$(CP) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OD) -h -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	$(NM) -n $< > $@

# Link: create ELF output file from object files.
.SECONDARY: $(TARGET).elf
.PRECIOUS: $(OBJ)
%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(CC) $(ALL_CFLAGS) $(OBJ) --output $@ $(LDFLAGS)

# Compile: create object files from C source files.
$(BUILD_DIR)%.o: $(SRC_DIR)%.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@

# Compile: create object files from C++ source files
$(BUILD_DIR)%.o: $(SRC_DIR)%.cpp
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CXXFLAGS) $< -o $@

# Assemble: create object files from assembler source files.
$(BUILD_DIR)%.o: $(SRC_DIR)%.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

# Target: clean project.
clean: begin clean_list finished end

clean_list :
	@echo
	@echo $(MSG_CLEANING)
	$(REMOVE) $(BUILD_DIR)*.elf
	$(REMOVE) $(BUILD_DIR)*.s
	$(REMOVE) $(BUILD_DIR)*.o
	$(REMOVE) $(BUILD_DIR)*.map
	$(REMOVE) $(BUILD_DIR)*.lst
	$(REMOVE) $(BUILD_DIR)*.eep
	$(REMOVE) $(BUILD_DIR)*.hex
	$(REMOVE) $(BUILD_DIR)*.lss
	$(REMOVE) $(BUILD_DIR)*.sym
	$(REMOVE) $(BUILD_DIR)*.d

# Listing of phony targets.
.PHONY : all begin finish end sizebefore sizeafter gccversion coff extcoff \
	clean clean_list program
