ARM_GNU ?= aarch64-linux-gnu
SD_BOOT ?= /Volumes/bootfs
FIRMWARE ?= /Users/noguchikoutarou/firmware/boot

MAIN_CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude
TRACE_CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude
USER_CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude
MAIN_ASMFLAGS = -Iinclude
TRACE_ASMFLAGS = -Iinclude
USER_ASMFLAGS = -Iinclude

MAIN_BUILD_DIR = main_build
TRACE_BUILD_DIR = trace_build
USER_BUILD_DIR = user_build
MAIN_SRC_DIR = src
TRACE_SRC_DIR = src/trace
USER_SRC_DIR = user_space

LINK_SCRIPT = src/linker.ld

define print_build
	@echo ""
	@echo "================================"
	@echo "Building$(1) ..."
	@echo "================================"
endef

all : kernel8.img armstub8.bin

clean :
	rm -rf $(MAIN_BUILD_DIR) $(TRACE_BUILD_DIR) $(USER_BUILD_DIR) *.img *.bin armstub/build
	scripts/clear_syms.py

# target in build dir, all files end in _c.o, showing they were created from a .c file
# dependency is in src, all the .c files
# $<: first dependency
# $@: target
$(MAIN_BUILD_DIR)/%_c.o: $(MAIN_SRC_DIR)/%.c
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(MAIN_CFLAGS) -MMD -c $< -o $@

$(MAIN_BUILD_DIR)/%_S.o: $(MAIN_SRC_DIR)/%.S
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(MAIN_ASMFLAGS) -MMD -c $< -o $@

$(TRACE_BUILD_DIR)/%_c.o: $(TRACE_SRC_DIR)/%.c
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(TRACE_CFLAGS) -MMD -c $< -o $@

$(TRACE_BUILD_DIR)/%_S.o: $(TRACE_SRC_DIR)/%.S
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(TRACE_ASMFLAGS) -MMD -c $< -o $@

$(USER_BUILD_DIR)/%_c.o: $(USER_SRC_DIR)/%.c
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(USER_CFLAGS) -MMD -c $< -o $@

$(USER_BUILD_DIR)/%_S.o: $(USER_SRC_DIR)/%.S
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(USER_ASMFLAGS) -MMD -c $< -o $@

MAIN_C_FILES = $(wildcard $(MAIN_SRC_DIR)/*.c)
TRACE_C_FILES = $(wildcard $(TRACE_SRC_DIR)/*.c)
USER_C_FILES = $(wildcard $(USER_SRC_DIR)/*.c)
MAIN_ASM_FILES = $(wildcard $(MAIN_SRC_DIR)/*.S)
TRACE_ASM_FILES = $(wildcard $(TRACE_SRC_DIR)/*.S)
USER_ASM_FILES = $(wildcard $(USER_SRC_DIR)/*.S)

# this means to go through every string in MAIN_C_FILES and if it matches
# $(MAIN_SRC_DIR)/%.c, change it to $(MAIN_BUILD_DIR)/%_c.o
OBJ_FILES = $(MAIN_C_FILES:$(MAIN_SRC_DIR)/%.c=$(MAIN_BUILD_DIR)/%_c.o)
OBJ_FILES += $(TRACE_C_FILES:$(TRACE_SRC_DIR)/%.c=$(TRACE_BUILD_DIR)/%_c.o)
OBJ_FILES += $(USER_C_FILES:$(USER_SRC_DIR)/%.c=$(USER_BUILD_DIR)/%_c.o)
OBJ_FILES += $(MAIN_ASM_FILES:$(MAIN_SRC_DIR)/%.S=$(MAIN_BUILD_DIR)/%_S.o)
OBJ_FILES += $(TRACE_ASM_FILES:$(TRACE_SRC_DIR)/%.S=$(TRACE_BUILD_DIR)/%_S.o)
OBJ_FILES += $(USER_ASM_FILES:$(USER_SRC_DIR)/%.S=$(USER_BUILD_DIR)/%_S.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(LINK_SCRIPT) $(OBJ_FILES)
	$(call print_build, $@)
	$(ARM_GNU)-ld -T $(LINK_SCRIPT) -o $(MAIN_BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARM_GNU)-objcopy $(MAIN_BUILD_DIR)/kernel8.elf -O binary $@

armstub/build/armstub8_S.o: armstub/src/armstub8.S
	$(call print_build, $@)
	mkdir -p $(@D)
	$(ARM_GNU)-gcc $(MAIN_CFLAGS) -MMD -c $< -o $@

armstub8.bin: armstub/build/armstub8_S.o
	$(call print_build, $@)
	$(ARM_GNU)-ld --section-start=.text=0 -o $(<D)/armstub8.elf $<
	$(ARM_GNU)-objcopy $(<D)/armstub8.elf -O binary $@

deploy: kernel8.img config.txt armstub8.bin
	rm -rf $(SD_BOOT)/*
	cp $^ $(SD_BOOT)/
	cp $(FIRMWARE)/bcm2711-rpi-4-b.dtb $(SD_BOOT)/
	cp $(FIRMWARE)/start4.elf $(SD_BOOT)/
	cp $(FIRMWARE)/fixup4.dat $(SD_BOOT)/
	mkdir $(SD_BOOT)/overlays
	cp $(FIRMWARE)/overlays/miniuart-bt.dtbo $(SD_BOOT)/overlays
	sync
