BOOTLOADER_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

BUILD_DIR = build/$(notdir $(BOARD_DIR))_bl

BOARD_CONFIG_HEADER = $(BOARD_DIR)/board.h
LDSCRIPT = $(BOARD_DIR)/board.ld

LIBOPENCM3_DIR := $(BOOTLOADER_DIR)/modules/libopencm3
LIBCANARD_DIR := $(BOOTLOADER_DIR)/modules/libcanard

ARCH_FLAGS := -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

LDFLAGS := --static -nostartfiles -L$(LIBOPENCM3_DIR)/lib -L$(dir $(LDSCRIPT)) -T$(LDSCRIPT) -Wl,--gc-sections --specs=nano.specs -u printf_float -Wl,--no-wchar-size-warning

LDLIBS := -lopencm3_stm32f3 -lm -Wl,--start-group -lc -lgcc -lrdimon -Wl,--end-group

CFLAGS += -std=gnu11 -Os -ffast-math -g -Wdouble-promotion -Wextra -Wshadow -Werror=implicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -fsingle-precision-constant -fno-common -ffunction-sections -fdata-sections -MD -Wall -Wundef -Isrc -I$(LIBOPENCM3_DIR)/include -I$(LIBCANARD_DIR) -Iinclude -DSTM32F3 -D"CANARD_ASSERT(x)"="do {} while(0)" -DGIT_HASH=0x$(shell git rev-parse --short=8 HEAD) -fshort-wchar -include $(BOARD_CONFIG_HEADER)

LIBOPENCM3_CFLAGS = -fshort-wchar
LIBOPENCM3_LDFLAGS =
LIBOPENCM3_AR = arm-none-eabi-gcc-ar

ifndef NO_LTO
  LDFLAGS += -flto
  CFLAGS += -flto
  LIBOPENCM3_CFLAGS += -flto
  LIBOPENCM3_LDFLAGS += -flto
endif

LIBOPENCM3_MAKE_ARGS = CFLAGS="$(LIBOPENCM3_CFLAGS)" LDFLAGS="$(LIBOPENCM3_LDFLAGS)" AR="$(LIBOPENCM3_AR)"

COMMON_OBJS := $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(shell find $(BOOTLOADER_DIR)/src -name "*.c"))))


ELF := $(BUILD_DIR)/bin/main.elf
BIN := $(BUILD_DIR)/bin/main.bin

.PHONY: all
all: $(LIBOPENCM3_DIR) $(BIN)

$(BUILD_DIR)/bin/%.elf: $(COMMON_OBJS) $(BUILD_DIR)/canard.o
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-gcc $(CFLAGS) $(LDFLAGS) $(ARCH_FLAGS) $^ $(LDLIBS) -o $@
	@arm-none-eabi-size $@

$(BUILD_DIR)/bin/%.bin: $(BUILD_DIR)/bin/%.elf
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-objcopy -O binary $< $@

.PRECIOUS: $(BUILD_DIR)/%.o
$(BUILD_DIR)/%.o: %.c $(LIBOPENCM3_DIR)
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-gcc $(CFLAGS) $(ARCH_FLAGS) -c $< -o $@

$(BUILD_DIR)/canard.o: $(LIBCANARD_DIR)/canard.c
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-gcc $(CFLAGS) $(ARCH_FLAGS) -c $< -o $@

.PHONY: $(LIBOPENCM3_DIR)
$(LIBOPENCM3_DIR):
	@echo "### BUILDING $@"
	@$(MAKE) -C $(LIBOPENCM3_DIR) $(LIBOPENCM3_MAKE_ARGS)

.PHONY: upload
upload: $(BUILD_DIR)/bin/main.elf $(BUILD_DIR)/bin/main.bin
	@echo "### UPLOADING"
	@openocd -f openocd.cfg -d2 -c "program $< verify reset exit"

.PHONY: clean
clean:
	@$(MAKE) -C $(LIBOPENCM3_DIR) clean
	@rm -rf build
