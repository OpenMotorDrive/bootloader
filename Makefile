LIBOPENCM3_DIR := omd_common/libopencm3
LIBCANARD_DIR := omd_common/libcanard

ARCH_FLAGS := -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

LDFLAGS := --static -nostartfiles -L$(LIBOPENCM3_DIR)/lib -L$(dir $(LDSCRIPT)) -T$(LDSCRIPT) -Wl,--gc-sections --specs=nano.specs -u printf_float -Wl,--no-wchar-size-warning

LDLIBS := -lopencm3_stm32f3 -lm -Wl,--start-group -lc -lgcc -lrdimon -Wl,--end-group

CFLAGS += -std=gnu11 -Os -ffast-math -g -Wdouble-promotion -Wextra -Wshadow -Werror=implicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -fsingle-precision-constant -fno-common -ffunction-sections -fdata-sections -MD -Wall -Wundef -Isrc -I$(LIBOPENCM3_DIR)/include -I$(LIBCANARD_DIR) -Iinclude -DSTM32F3 -D"CANARD_ASSERT(x)"="do {} while(0)" -DGIT_HASH=0x$(shell git rev-parse --short=8 HEAD) -fshort-wchar

ifdef USE_LTO
	LDFLAGS += -flto
	CFLAGS += -flto
	LIBOPENCM3_MAKE_ARGS = CFLAGS="-fshort-wchar -flto" LDFLAGS="-flto" AR="arm-none-eabi-gcc-ar"
else
	LIBOPENCM3_MAKE_ARGS = CFLAGS="-fshort-wchar"
endif

COMMON_OBJS := $(addprefix build/,$(addsuffix .o,$(basename $(shell find src -name "*.c"))))
COMMON_OBJS += $(addprefix build/,$(addsuffix .o,$(basename $(shell find shared -name "*.c"))))

BOARD_CONFIG_OBJ := $(addprefix build/build/,$(addsuffix .o,$(notdir $(basename $(BOARD_CONFIG_FILE)))))

ELF := build/bin/main.elf
BIN := build/bin/main.bin

.PHONY: all
all: $(LIBOPENCM3_DIR) $(BIN)

build/bin/%.elf: $(BOARD_CONFIG_OBJ) $(COMMON_OBJS) build/canard.o
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-gcc $(CFLAGS) $(LDFLAGS) $(ARCH_FLAGS) $^ $(LDLIBS) -o $@
	@arm-none-eabi-size $@

build/bin/%.bin: build/bin/%.elf
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-objcopy -O binary $< $@

$(addprefix build/,$(notdir $(BOARD_CONFIG_FILE))): $(BOARD_CONFIG_FILE)
	@mkdir -p "$(dir $@)"
	@cp $< $@

.PRECIOUS: build/%.o
build/%.o: %.c $(LIBOPENCM3_DIR)
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-gcc $(CFLAGS) $(ARCH_FLAGS) -S $< -o $(patsubst %.o,%.S,$@)
	@arm-none-eabi-gcc $(CFLAGS) $(ARCH_FLAGS) -c $< -o $@

build/canard.o: $(LIBCANARD_DIR)/canard.c
	@echo "### BUILDING $@"
	@mkdir -p "$(dir $@)"
	@arm-none-eabi-gcc $(CFLAGS) $(ARCH_FLAGS) -c $< -o $@

.PHONY: $(LIBOPENCM3_DIR)
$(LIBOPENCM3_DIR):
	@echo "### BUILDING $@"
	@$(MAKE) -C $(LIBOPENCM3_DIR) $(LIBOPENCM3_MAKE_ARGS)

.PHONY: upload
upload: build/bin/main.elf build/bin/main.bin
	@echo "### UPLOADING"
	@openocd -f openocd.cfg -c "program $< verify reset exit"

.PHONY: clean
clean:
	@$(MAKE) -C $(LIBOPENCM3_DIR) clean
	@rm -rf build
