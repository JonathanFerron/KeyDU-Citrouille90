# =============================================================================
# KeyDU-Citrouille90 Master Makefile
# Builds KeyDU.App (HID keyboard) and KeyDU.BL (vendor-class bootloader)
# for the AVR64DU32.
#
# Usage:
#   make or make all   — build both targets
#   make app       — build KeyDU.App only
#   make bl        — build KeyDU.BL only
#   make merged    - build the merged KeyDU.BL+KeyDU.App Hex file
#   make flash_app    — program KeyDU.App via UPDI
#   make flash_bl     — program KeyDU.BL via UPDI
#   make flash_fuses  — program fuse row via UPDI (from BL hex)
#   make flash_merged  — program merged KeyDU.BL+App via UPDI (full reflash)
#   make flash_usb  — program KeyDU.App via USB vendor protocol
#   make clean     — remove all build artefacts
#   make size      — print section sizes for both targets
#   make bear      — regenerate compile_commands.json
#   make format    — format all .c/.h files with astyle
#   make help      — print available targets
# =============================================================================

MAKEFLAGS += -j$(shell nproc --ignore=2)

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------
CC      = avr-gcc
AS      = avr-gcc          # assembles .S files through the C driver
OBJCOPY = avr-objcopy
SIZE    = avr-size
AVRDUDE = avrdude

# -----------------------------------------------------------------------------
# Target hardware
# -----------------------------------------------------------------------------
MCU   = avr64du32
F_CPU = 24000000UL

# -----------------------------------------------------------------------------
# Programmer (Adafruit UPDI Friend at 5V)
# -----------------------------------------------------------------------------
PROG_ID   = serialupdi
PROG_PORT = /dev/ttyUSB0   # CH340 (ch341 driver) — confirmed ttyUSB0 when it is the only USB-UART device; check dmesg | tail on plug-in if others are attached

# -----------------------------------------------------------------------------
# Build output directory
# -----------------------------------------------------------------------------
BUILD = build

# =============================================================================
# Common compiler / assembler flags
# =============================================================================
COMMON_CFLAGS  = -mmcu=$(MCU)
COMMON_CFLAGS += -DF_CPU=$(F_CPU)
COMMON_CFLAGS += -Os
COMMON_CFLAGS += -std=gnu11              # upgraded from gnu99; makes _Static_assert explicit
COMMON_CFLAGS += -Wall
COMMON_CFLAGS += -Wextra                 # unused params, missing initialisers, sign-compare
COMMON_CFLAGS += -Wshadow                # locals shadowing outer-scope names
COMMON_CFLAGS += -Wundef                 # #if on undefined macro (typo guard)
COMMON_CFLAGS += -Wformat=2              # stricter format-string checking
COMMON_CFLAGS += -Wstrict-prototypes
COMMON_CFLAGS += -ffunction-sections
COMMON_CFLAGS += -fdata-sections
# Removed (applied selectively via USB_PACKED instead) COMMON_CFLAGS += -fpack-struct
COMMON_CFLAGS += -fno-strict-aliasing    # pointer-cast-heavy USB code; avoids aliasing UB
COMMON_CFLAGS += -fshort-enums
COMMON_CFLAGS += -mrodata-in-ram

COMMON_ASFLAGS  = -mmcu=$(MCU)
COMMON_ASFLAGS += -DF_CPU=$(F_CPU)
COMMON_ASFLAGS += -x assembler-with-cpp
COMMON_ASFLAGS += -Wa,-adhlns=$(patsubst %.S,$(BUILD)/%.lst,$<)  # listing file in build tree

COMMON_LDFLAGS  = -mmcu=$(MCU)
COMMON_LDFLAGS += -Wl,--gc-sections
COMMON_LDFLAGS += -Wl,--relax
COMMON_LDFLAGS += -mrodata-in-ram

# =============================================================================
# KeyDU.App — HID keyboard application
# =============================================================================

APP_DIR  = KeyDU.App
APP_OUT  = $(BUILD)/app
APP_NAME = KeyDU.App
APP_ELF  = $(APP_OUT)/$(APP_NAME).elf
APP_HEX  = $(APP_OUT)/$(APP_NAME).hex

APP_INC  = -I$(APP_DIR) -Iusbcore -I$(APP_DIR)/usbhid -Iavrducore

APP_C_SRC = \
    $(APP_DIR)/app_main.c       \
    $(APP_DIR)/keyboard.c       \
    $(APP_DIR)/matrix.c         \
    $(APP_DIR)/keymap.c         \
    $(APP_DIR)/layer.c          \
    $(APP_DIR)/macro.c          \
    $(APP_DIR)/compose.c        \
    $(APP_DIR)/led.c            \
    $(APP_DIR)/encoder.c        \
    $(APP_DIR)/usb_hid.c        \
    $(APP_DIR)/usb_desc.c       \
    usbcore/usb_ctrl.c          \
    usbcore/usb_ep.c            \
    usbcore/usb_ep_stream.c     \
    avrducore/clock.c

APP_S_SRC = \
    avrducore/ccp.S

APP_CFLAGS  = $(COMMON_CFLAGS) $(APP_INC) -DEP_TABLE_COUNT=3 -MMD -MP   # EP0 + EP1 kbd + EP2 consumer; update to 6 for v2
APP_ASFLAGS = $(COMMON_ASFLAGS) $(APP_INC)
APP_LDFLAGS = $(COMMON_LDFLAGS) -Wl,--section-start=.text=0x2000

APP_C_OBJ = $(patsubst %.c,  $(APP_OUT)/%.o, $(APP_C_SRC))
APP_S_OBJ = $(patsubst %.S,  $(APP_OUT)/%.o, $(APP_S_SRC))
APP_OBJ   = $(APP_C_OBJ) $(APP_S_OBJ)

# =============================================================================
# KeyDU.BL — vendor-class bootloader
# =============================================================================

BL_DIR  = KeyDU.BL
BL_OUT  = $(BUILD)/bl
BL_NAME = KeyDU.BL
BL_ELF  = $(BL_OUT)/$(BL_NAME).elf
BL_HEX  = $(BL_OUT)/$(BL_NAME).hex

BL_INC  = -I$(BL_DIR) -Iusbcore -I$(BL_DIR)/usbvendor -Iavrducore

BL_C_SRC = \
    $(BL_DIR)/bl_main.c                   \
    $(BL_DIR)/usb_vendor.c      \
    $(BL_DIR)/usb_vendor_desc.c \
    $(BL_DIR)/flash.c                     \
    usbcore/usb_ctrl.c                    \
    usbcore/usb_ep.c                      \
    usbcore/usb_ep_stream.c               \
    avrducore/fuses.c           \
    avrducore/clock.c

BL_S_SRC = \
    avrducore/ccp.S

BL_CFLAGS  = $(COMMON_CFLAGS) $(BL_INC) -DEP_TABLE_COUNT=1 -MMD -MP   # EP0 only; DFU BL will also need only 1
BL_ASFLAGS = $(COMMON_ASFLAGS) $(BL_INC)
BL_LDFLAGS = $(COMMON_LDFLAGS)

BL_C_OBJ = $(patsubst %.c,  $(BL_OUT)/%.o, $(BL_C_SRC))
BL_S_OBJ = $(patsubst %.S,  $(BL_OUT)/%.o, $(BL_S_SRC))
BL_OBJ   = $(BL_C_OBJ) $(BL_S_OBJ)

MERGED_HEX = $(BUILD)/KeyDU.merged.hex

# Include GCC-generated header dependency files (-MMD -MP).
# The leading dash silently skips missing files (first build, after make clean).
-include $(APP_C_OBJ:.o=.d)
-include $(BL_C_OBJ:.o=.d)

# =============================================================================
# Top-level targets
# =============================================================================

.PHONY: all app bl clean size flash_merged flash_app flash_bl flash_fuses flash_usb flasher bear format help merged

all: app bl

app: $(APP_HEX)
	@echo ""
	@echo "=== KeyDU.App size ==="
	$(SIZE) --format=avr --mcu=$(MCU) $(APP_ELF)

bl: $(BL_HEX)
	@echo ""
	@echo "=== KeyDU.BL size ==="
	$(SIZE) --format=avr --mcu=$(MCU) $(BL_ELF)

size: app bl

merged: app bl $(MERGED_HEX)
	@echo ""
	@echo "=== Merged image ==="
	$(SIZE) --format=avr --mcu=$(MCU) $(APP_ELF)
	$(SIZE) --format=avr --mcu=$(MCU) $(BL_ELF)

flasher: $(FLASHER_BIN)

# =============================================================================
# Link
# =============================================================================

$(APP_ELF): $(APP_OBJ)
	@mkdir -p $(dir $@)
	@echo "[LD]  $@"
	$(CC) $(APP_LDFLAGS) $^ -o $@

$(BL_ELF): $(BL_OBJ)
	@mkdir -p $(dir $@)
	@echo "[LD]  $@"
	$(CC) $(BL_LDFLAGS) $^ -o $@

# =============================================================================
# Hex
# =============================================================================

$(APP_HEX): $(APP_ELF)
	@echo "[HEX] $@"
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BL_HEX): $(BL_ELF)
	@echo "[HEX] $@"
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# =============================================================================
# Compile: .c → .o  (App)
# =============================================================================

$(APP_OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC]  $<"
	$(CC) $(APP_CFLAGS) -c $< -o $@

# =============================================================================
# Compile: .c → .o  (BL)
# =============================================================================

$(BL_OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC]  $<"
	$(CC) $(BL_CFLAGS) -c $< -o $@

# =============================================================================
# Assemble: .S → .o  (shared avrducore — built separately for each target)
# =============================================================================

$(APP_OUT)/%.o: %.S
	@mkdir -p $(dir $@)
	@mkdir -p $(BUILD)/avrducore
	@echo "[AS]  $<"
	$(AS) $(APP_ASFLAGS) -c $< -o $@

$(BL_OUT)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "[AS]  $<"
	$(AS) $(BL_ASFLAGS) -c $< -o $@

# =============================================================================
# Merged hex (BL + App combined for full UPDI flash)
# =============================================================================

$(MERGED_HEX): $(BL_HEX) $(APP_HEX)
	@mkdir -p $(BUILD)
	@echo "[MERGE] $@"
	srec_cat $(BL_HEX) -Intel $(APP_HEX) -Intel -o $@ -Intel

# =============================================================================
# Flasher tool (keydu_flasher — USB vendor protocol)
# =============================================================================

FLASHER_DIR = tools
FLASHER_SRC = $(FLASHER_DIR)/keydu_flasher.c
FLASHER_BIN = $(BUILD)/keydu_flasher

$(FLASHER_BIN): $(FLASHER_SRC)
	@mkdir -p $(BUILD)
	@echo "[CC]  $@"
	gcc -o $@ $< $(shell pkg-config --cflags --libs libusb-1.0)

# =============================================================================
# Flash targets
# =============================================================================

# Full UPDI flash — merged BL + App in one shot.
# Use this for initial bring-up and any time the bootloader changes.
flash_merged: $(MERGED_HEX)
	@echo "Flashing merged image (BL + App) via UPDI..."
	$(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -U flash:w:$<:i

# UPDI app-only — for development when BL is already on the chip.
# Uses -D to suppress the auto-erase so the bootloader partition
# is not disturbed. Only safe if the app region was previously erased
# (i.e. a full flash_updi was done at least once before).
flash_app: $(APP_HEX)
	@echo "Flashing KeyDU.App only via UPDI (no erase)..."
	$(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -D -U flash:w:$<:i

# UPDI BL-only — reflash bootloader without touching app.
# Same -D caveat as flash_app above.
flash_bl: $(BL_HEX)
	@echo "Flashing KeyDU.BL only via UPDI (no erase)..."
	$(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -D -U flash:w:$<:i

# UPDI fuses-only — program the fuse row from the BL hex. fuses.c emits the
# FUSES struct to the .fuse section at 0x820000 in KeyDU.BL.elf; avrdude 8.x
# recognises the multi-memory hex and writes only the fuse segment. Run once
# at bring-up and after any fuses.c change. No -D needed: a fuses-only write
# performs no flash erase. (Requires avrdude >= 8.0 for multi-memory hex.)
flash_fuses: $(BL_HEX)
	@echo "Flashing fuse row via UPDI..."
	$(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -U fuses:w:$<:i

# USB flash — send app image to running bootloader via vendor protocol.
# Device must already be in bootloader mode (SYS_BOOT key).
flash_usb: $(APP_HEX) $(FLASHER_BIN)
	@echo "Flashing KeyDU.App via USB vendor protocol..."
	$(OBJCOPY) -I ihex -O binary $(APP_HEX) $(APP_OUT)/KeyDU.App.bin
	$(FLASHER_BIN) $(APP_OUT)/KeyDU.App.bin

# =============================================================================
# compile_commands.json (bear)
# Run after adding or removing source files.
# Uses $(MAKE) so bear sees the same make binary and jobserver flags.
# bear intercepts compiler calls during a full rebuild — run make clean first
# or use: bear -- $(MAKE) -B all
# =============================================================================

bear:
	bear -- $(MAKE) all

# =============================================================================
# Source formatting (astyle)
# Explicit per-directory wildcards keep build/ and any future vendored
# subdirectories out of scope. --project reads .astylerc at the project
# root automatically. --suffix=none suppresses .orig backup files.
# =============================================================================

FORMAT_SRC = \
    $(wildcard $(APP_DIR)/*.c)  \
    $(wildcard $(APP_DIR)/*.h)  \
    $(wildcard $(BL_DIR)/*.c)   \
    $(wildcard $(BL_DIR)/*.h)   \
    $(wildcard usbcore/*.c)     \
    $(wildcard usbcore/*.h)     \
    $(wildcard avrducore/*.c)   \
    $(wildcard avrducore/*.h)
 
format:
	astyle --project --suffix=none $(FORMAT_SRC)

# =============================================================================
# Help
# =============================================================================

help:
	@echo "KeyDU-Citrouille90 — AVR64DU32 keyboard firmware"
	@echo ""
	@echo "Build targets:"
	@echo "  all          Build both KeyDU.App and KeyDU.BL (default)"
	@echo "  app          Build KeyDU.App (HID keyboard) only"
	@echo "  bl           Build KeyDU.BL (vendor bootloader) only"
	@echo "  merged       Build merged BL+App hex image"
	@echo "  size         Build both and print section sizes"
	@echo ""
	@echo "Flash targets:"
	@echo "  flash_merged Program merged BL+App image via UPDI (full reflash)"
	@echo "  flash_app    Program KeyDU.App only via UPDI (no erase)"
	@echo "  flash_bl     Program KeyDU.BL only via UPDI (no erase)"
	@echo "  flash_fuses  Program fuse row only via UPDI (from BL hex)"
	@echo "  flash_usb    Program KeyDU.App via USB vendor protocol"
	@echo ""
	@echo "Dev targets:"
	@echo "  bear         Regenerate compile_commands.json (run make clean first)"
	@echo "  format       Format all .c/.h files with astyle"
	@echo "  clean        Remove all build artefacts"
	@echo "  help         Show this message"
	@echo ""
	@echo "Configuration:"
	@echo "  MCU:         $(MCU)"
	@echo "  F_CPU:       $(F_CPU)"
	@echo "  Compiler:    $(CC)"
	@echo "  Programmer:  $(PROG_ID) on $(PROG_PORT)"
	@echo "  Build dir:   $(BUILD)"

# =============================================================================
# Clean
# =============================================================================

clean:
	@echo "Removing build artefacts..."
	rm -rf $(BUILD)
	@echo "Done."
