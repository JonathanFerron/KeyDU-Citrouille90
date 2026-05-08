# =============================================================================
# KeyDU-Citrouille90 Master Makefile
# Builds KeyDU.App (HID keyboard) and KeyDU.BL (vendor-class bootloader)
# for the AVR64DU32.
#
# Usage:
#   make           — build both targets
#   make app       — build KeyDU.App only
#   make bl        — build KeyDU.BL only
#   make flash_app — program KeyDU.App via UPDI
#   make flash_bl  — program KeyDU.BL via UPDI
#   make clean     — remove all build artefacts
#   make size      — print section sizes for both targets
# =============================================================================

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
PROG_PORT = /dev/ttyUSB0   # adjust for your system

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
COMMON_CFLAGS += -std=gnu99
COMMON_CFLAGS += -Wall
COMMON_CFLAGS += -Wstrict-prototypes
COMMON_CFLAGS += -ffunction-sections
COMMON_CFLAGS += -fdata-sections
COMMON_CFLAGS += -fpack-struct
COMMON_CFLAGS += -fshort-enums

COMMON_ASFLAGS  = -mmcu=$(MCU)
COMMON_ASFLAGS += -DF_CPU=$(F_CPU)
COMMON_ASFLAGS += -x assembler-with-cpp

COMMON_LDFLAGS  = -mmcu=$(MCU)
COMMON_LDFLAGS += -Wl,--gc-sections
COMMON_LDFLAGS += -Wl,--relax

# =============================================================================
# KeyDU.App — HID keyboard application
# =============================================================================

APP_DIR  = KeyDU.App
APP_OUT  = $(BUILD)/app
APP_NAME = KeyDU.App
APP_ELF  = $(APP_OUT)/$(APP_NAME).elf
APP_HEX  = $(APP_OUT)/$(APP_NAME).hex
APP_LD   = $(APP_DIR)/KeyDU.App.ld

APP_INC  = -I$(APP_DIR) -Iusbcore -I$(APP_DIR)/usbhid -Iavrducore

APP_C_SRC = \
    $(APP_DIR)/app_main.c           \
    $(APP_DIR)/keyboard.c       \
    $(APP_DIR)/matrix.c         \
    $(APP_DIR)/keymap.c         \
    $(APP_DIR)/layer.c          \
    $(APP_DIR)/macro.c          \
    $(APP_DIR)/led.c            \
    $(APP_DIR)/encoder.c        \
    $(APP_DIR)/usb_hid.c \
    $(APP_DIR)/usb_desc.c \
    usbcore/usb_ctrl.c          \
    usbcore/usb_ep.c            \
    usbcore/usb_ep_stream.c     \
    avrducore/fuses.c          \
    avrducore/clock.c

APP_S_SRC = \
    avrducore/ccp.S

APP_CFLAGS  = $(COMMON_CFLAGS) $(APP_INC)
APP_ASFLAGS = $(COMMON_ASFLAGS) $(APP_INC)
APP_LDFLAGS = $(COMMON_LDFLAGS) -T$(APP_LD)

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
BL_LD   = $(BL_DIR)/KeyDU.BL.ld

BL_INC  = -I$(BL_DIR) -Iusbcore -I$(BL_DIR)/usbvendor -Iavrducore

BL_C_SRC = \
    $(BL_DIR)/bl_main.c                   \
    $(BL_DIR)/usb_vendor.c      \
    $(BL_DIR)/usb_vendor_desc.c \
    $(BL_DIR)/flash.c                     \
    usbcore/usb_ctrl.c                    \
    usbcore/usb_ep.c                      \
    usbcore/usb_ep_stream.c               \
    avrducore/clock.c

BL_S_SRC = \
    avrducore/ccp.S

BL_CFLAGS  = $(COMMON_CFLAGS) $(BL_INC)
BL_ASFLAGS = $(COMMON_ASFLAGS) $(BL_INC)
BL_LDFLAGS = $(COMMON_LDFLAGS) -T$(BL_LD)

BL_C_OBJ = $(patsubst %.c,  $(BL_OUT)/%.o, $(BL_C_SRC))
BL_S_OBJ = $(patsubst %.S,  $(BL_OUT)/%.o, $(BL_S_SRC))
BL_OBJ   = $(BL_C_OBJ) $(BL_S_OBJ)

# =============================================================================
# Top-level targets
# =============================================================================

.PHONY: all app bl clean size flash_app flash_bl

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
	@echo "[AS]  $<"
	$(AS) $(APP_ASFLAGS) -c $< -o $@

$(BL_OUT)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "[AS]  $<"
	$(AS) $(BL_ASFLAGS) -c $< -o $@

# =============================================================================
# Flash
# =============================================================================

flash_app: $(APP_HEX)
	@echo "Flashing KeyDU.App..."
	$(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -U flash:w:$<:i

flash_bl: $(BL_HEX)
	@echo "Flashing KeyDU.BL..."
	$(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -U flash:w:$<:i

# =============================================================================
# Clean
# =============================================================================

clean:
	@echo "Removing build artefacts..."
	rm -rf $(BUILD)
	@echo "Done."
