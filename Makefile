######################################
# STM32F10x Makefile
######################################

######################################
# target
######################################
TARGET = Blank

######################################
# building variables
######################################
# debug build?
DEBUG = 1
# build for debug in ram?
RAMBUILD = 0
# optimization
OPT = -O0
#OPT = -Os

#######################################
# paths
#######################################
# source path
VPATH = src startup
# firmware library path
PERIPHLIBPATH = ./periphlib
#VPATH += $(PERIPHLIBPATH)/CMSIS/CM3/CoreSupport
#VPATH += $(PERIPHLIBPATH)/CMSIS/CM3/DeviceSupport/ST/STM32F10x
#VPATH += $(PERIPHLIBPATH)/STM32F10x_StdPeriph_Driver/src

VPATH += $(PERIPHLIBPATH)/CMSIS/Device/ST/STM32F30x
VPATH += $(PERIPHLIBPATH)/STM32F30x_StdPeriph_Driver/src
# Build path
BUILD_DIR = build

# #####################################
# source
# #####################################
SRCS = \
  main.c \
  stm32f30x_it.c \
  system_stm32f30x.c
 
SRCSASM = \
  startup_stm32f30x.s 

# #####################################
# firmware library
# #####################################
#   core_cm0.c 
PERIPHLIB_SOURCES = \
  stm32f30x_gpio.c \
  stm32f30x_rcc.c \
  stm32f30x_tim.c \
  stm32f30x_dma.c \
  stm32f30x_rtc.c

#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
CP = arm-none-eabi-objcopy
AR = arm-none-eabi-ar
SZ = arm-none-eabi-size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# macros for gcc
DEFS = -DSTM32F30x -DUSE_STDPERIPH_DRIVER
ifeq ($(RAMBUILD), 1)
DEFS += -DVECT_TAB_SRAM
endif
ifeq ($(DEBUG), 1)
DEFS += -DDEBUG -D_DEBUG
endif
# includes for gcc
INCLUDES = -Iinc
# INCLUDES += -I$(PERIPHLIBPATH)/CMSIS/CM3/CoreSupport
# INCLUDES += -I$(PERIPHLIBPATH)/CMSIS/CM3/DeviceSupport/ST/STM32F10x
# INCLUDES += -I$(PERIPHLIBPATH)/STM32F10x_StdPeriph_Driver/inc
INCLUDES += -I$(PERIPHLIBPATH)/CMSIS/Device/ST/STM32F30x/Include
INCLUDES += -I$(PERIPHLIBPATH)/STM32F30x_StdPeriph_Driver/inc
INCLUDES += -I$(PERIPHLIBPATH)/CMSIS/Include
# compile gcc flags
CFLAGS = -mthumb -mcpu=cortex-m4 $(DEFS) $(INCLUDES) $(OPT) -Wall
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2 -fno-common -fdata-sections -ffunction-sections
endif
# Generate dependency information
CFLAGS += -MD -MP -MF .dep/$(@F).d

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = linker/STM32F303VC_FLASH.ld
# libraries
LIBS = -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = -mthumb -mcpu=cortex-m4 -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections


# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex


#######################################
# build the application
#######################################
# list of firmware library objects
PERIPHLIB_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(PERIPHLIB_SOURCES:.c=.o)))
# list of C program objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(SRCSASM:.s=.o)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(PERIPHLIB_OBJECTS) Makefile
	$(CC) $(OBJECTS) $(PERIPHLIB_OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@
	
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR):
	mkdir -p $@


#######################################
# delete all user application files
#######################################
clean:
	-rm -fR .dep $(BUILD_DIR)
  
#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
