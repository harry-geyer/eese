#Toolchain settings
TOOLCHAIN := arm-none-eabi

CC = $(TOOLCHAIN)-gcc
OBJCOPY = $(TOOLCHAIN)-objcopy
OBJDUMP = $(TOOLCHAIN)-objdump
SIZE = $(TOOLCHAIN)-size
AR = $(TOOLCHAIN)-ar

#Target CPU options
CPU_DEFINES = -mthumb -mcpu=cortex-m0 -DSTM32F0 -pedantic

GIT_COMMITS != git rev-list --count HEAD
GIT_COMMIT != git log -n 1 --format="%h-%f"

#Compiler options
CFLAGS		+= -Os -g -std=gnu11
CFLAGS		+= -Wall -Wextra -Werror -Wno-unused-parameter -Wno-address-of-packed-member
CFLAGS		+= -fstack-usage -Wstack-usage=100
CFLAGS		+= -MMD -MP
CFLAGS		+= -fno-common -ffunction-sections -fdata-sections
CFLAGS		+= $(CPU_DEFINES) --specs=picolibc.specs -flto
CFLAGS		+= -DGIT_VERSION=\"[$(GIT_COMMITS)]-$(GIT_COMMIT)\"

INCLUDE_DIR = include
INCLUDE_PATHS += -Ilibs/libopencm3/include -I$(INCLUDE_DIR)
INCLUDE_PATHS += -Ilibs/nanocobs

RESOURCE_DIR = resources
LINK_SCRIPT = $(RESOURCE_DIR)/stm32f07xzb.ld

LINK_FLAGS =  -Llibs/libopencm3/lib --static -nostartfiles
LINK_FLAGS += -Llibs/libopencm3/lib/stm32/f0
LINK_FLAGS += build/libs/nanocobs/nanocobs.a
LINK_FLAGS += -T$(LINK_SCRIPT) -lopencm3_stm32f0
LINK_FLAGS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group -Wl,--gc-sections
LINK_FLAGS += $(CPU_DEFINES) --specs=picolibc.specs

SOURCE_DIR = src
SOURCES += $(shell find "$(SOURCE_DIR)" -type f -name "*.c")

BUILD_DIR := build
PROJECT_NAME := firmware

OBJECTS_DIR = $(BUILD_DIR)/objs
OBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(OBJECTS_DIR)/%.o,$(SOURCES))
DEPS = $(patsubst $(SOURCE_DIR)/%.c,$(OBJECTS_DIR)/%.d,$(SOURCES))
TARGET_ELF = $(BUILD_DIR)/$(PROJECT_NAME).elf
TARGET_HEX = $(TARGET_ELF:%.elf=%.hex)
TARGET_BIN = $(TARGET_ELF:%.elf=%.bin)
TARGET_DFU = $(TARGET_ELF:%.elf=%.dfu)

LIBOPENCM3 := libs/libopencm3/lib/libopencm3_stm32f0.a
NANOCOBS := build/libs/nanocobs/nanocobs.a

default: $(TARGET_BIN)

$(BUILD_DIR)/.git.$(GIT_COMMIT):
	mkdir -p $(@D)
	rm -f $(BUILD_DIR)/.git.*
	touch $@

$(TARGET_ELF): $(LIBS) $(OBJECTS) $(LINK_SCRIPT) $(NANOCOBS)
	$(CC) $(OBJECTS) $(LINK_FLAGS) -o $(TARGET_ELF)

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(OBJECTS): $(OBJECTS_DIR)/%.o: $(SOURCE_DIR)/%.c $(BUILD_DIR)/.git.$(GIT_COMMIT) $(LIBOPENCM3)
	mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(INCLUDE_PATHS) $< -o $@

$(LIBOPENCM3):
	$(MAKE) -C libs/libopencm3 TARGETS=stm32/f0

size: $(TARGET_ELF)
	$(SIZE) $(TARGET_ELF)

clean:
	rm -rf $(BUILD_DIR)/
	$(MAKE) -C libs/libopencm3 TARGETS=stm32/f0 clean
	$(MAKE) -C libs/nanocobs clean

cppcheck:
	cppcheck --enable=all --std=c99 $(INCLUDE_PATHS) --report-type=misra-c-2023 --addon=$(RESOURCE_DIR)/misra.json --suppress=missingIncludeSystem --check-level=exhaustive $(SOURCES)

$(BUILD_DIR)/stack_info : $(TARGET_ELF)
	./avstack.pl $(OBJECTS) > $@

stack_info: $(BUILD_DIR)/stack_info
	cat $(BUILD_DIR)/stack_info

.PHONY: size clean cppcheck stack_info test

include libs/nanocobs.mk
include tests/tests.mk
-include $(DEPS)
