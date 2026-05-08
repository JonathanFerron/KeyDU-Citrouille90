// keycode.h - Citrouille90 Keyboard Firmware
// Keycode definitions and type checking macros

#ifndef KEYCODE_H
#define KEYCODE_H

#include <stdint.h>

// TODO: Add some explanation here of the range system and how to add a keycode

// ============================================================================
// Basic HID Keyboard codes (Page 0x07) - values 0x00 to 0xFF
// Prefix: KC_ (KeyCode)
// ============================================================================
#define KC_NO           0x00  // No key pressed
#define KC_TRNS         0x01  // Transparent - fall through to lower layer

// Letters
#define KC_A            0x04
#define KC_B            0x05
#define KC_C            0x06
#define KC_D            0x07
#define KC_E            0x08
#define KC_F            0x09
#define KC_G            0x0A
#define KC_H            0x0B
#define KC_I            0x0C
#define KC_J            0x0D
#define KC_K            0x0E
#define KC_L            0x0F
#define KC_M            0x10
#define KC_N            0x11
#define KC_O            0x12
#define KC_P            0x13
#define KC_Q            0x14
#define KC_R            0x15
#define KC_S            0x16
#define KC_T            0x17
#define KC_U            0x18
#define KC_V            0x19
#define KC_W            0x1A
#define KC_X            0x1B
#define KC_Y            0x1C
#define KC_Z            0x1D

// Numbers
#define KC_1            0x1E
#define KC_2            0x1F
#define KC_3            0x20
#define KC_4            0x21
#define KC_5            0x22
#define KC_6            0x23
#define KC_7            0x24
#define KC_8            0x25
#define KC_9            0x26
#define KC_0            0x27

// Special keys
#define KC_ENT          0x28  // Enter
#define KC_ESC          0x29  // Escape
#define KC_BSPC         0x2A  // Backspace
#define KC_TAB          0x2B  // Tab
#define KC_SPC          0x2C  // Space
#define KC_MINS         0x2D  // - and _
#define KC_EQL          0x2E  // = and +
#define KC_LBRC         0x2F  // [ and {
#define KC_RBRC         0x30  // ] and }
#define KC_BSLS         0x31  // \ and |
#define KC_SCLN         0x33  // ; and :
#define KC_QUOT         0x34  // ' and "
#define KC_GRV          0x35  // ` and ~
#define KC_COMM         0x36  // , and <
#define KC_DOT          0x37  // . and >
#define KC_SLSH         0x38  // / and ?
#define KC_CAPS         0x39  // Caps Lock

// Function keys
#define KC_F1           0x3A
#define KC_F2           0x3B
#define KC_F3           0x3C
#define KC_F4           0x3D
#define KC_F5           0x3E
#define KC_F6           0x3F
#define KC_F7           0x40
#define KC_F8           0x41
#define KC_F9           0x42
#define KC_F10          0x43
#define KC_F11          0x44
#define KC_F12          0x45

// Navigation
#define KC_PSCR         0x46  // Print Screen
#define KC_SLCK         0x47  // Scroll Lock
#define KC_PAUS         0x48  // Pause
#define KC_INS          0x49  // Insert
#define KC_HOME         0x4A
#define KC_PGUP         0x4B  // Page Up
#define KC_DEL          0x4C  // Delete
#define KC_END          0x4D
#define KC_PGDN         0x4E  // Page Down
#define KC_RGHT         0x4F  // Right Arrow
#define KC_LEFT         0x50  // Left Arrow
#define KC_DOWN         0x51  // Down Arrow
#define KC_UP           0x52  // Up Arrow

// Keypad
#define KC_NLCK         0x53  // Num Lock
#define KC_PSLS         0x54  // Keypad /
#define KC_PAST         0x55  // Keypad *
#define KC_PMNS         0x56  // Keypad -
#define KC_PPLS         0x57  // Keypad +
#define KC_PENT         0x58  // Keypad Enter
#define KC_P1           0x59  // Keypad 1
#define KC_P2           0x5A  // Keypad 2
#define KC_P3           0x5B  // Keypad 3
#define KC_P4           0x5C  // Keypad 4
#define KC_P5           0x5D  // Keypad 5
#define KC_P6           0x5E  // Keypad 6
#define KC_P7           0x5F  // Keypad 7
#define KC_P8           0x60  // Keypad 8
#define KC_P9           0x61  // Keypad 9
#define KC_P0           0x62  // Keypad 0
#define KC_PDOT         0x63  // Keypad .

// Modifiers
#define KC_LCTL         0xE0  // Left Control
#define KC_LSFT         0xE1  // Left Shift
#define KC_LALT         0xE2  // Left Alt
#define KC_LGUI         0xE3  // Left GUI (Windows/Command)
#define KC_RCTL         0xE4  // Right Control
#define KC_RSFT         0xE5  // Right Shift
#define KC_RALT         0xE6  // Right Alt
#define KC_RGUI         0xE7  // Right GUI

// Modifier bit positions for HID report
#define MOD_LCTL        0x01
#define MOD_LSFT        0x02
#define MOD_LALT        0x04
#define MOD_LGUI        0x08
#define MOD_RCTL        0x10
#define MOD_RSFT        0x20
#define MOD_RALT        0x40
#define MOD_RGUI        0x80

// Helper macro to convert KC_Lxxx to MOD_Lxxx bit
#define MOD_BIT(kc)     (1 << ((kc) & 0x07))

// ============================================================================
// Consumer Control keys (Page 0x0C) - Range 0x0100-0x01FF
// Prefix: CC_ (Consumer Control)
// ============================================================================
#define CC_BASE         0x0100
#define CC_MUTE         (CC_BASE | 0x01)
#define CC_VOLU         (CC_BASE | 0x02)  // Volume Up
#define CC_VOLD         (CC_BASE | 0x03)  // Volume Down
#define CC_MNXT         (CC_BASE | 0x04)  // Next track
#define CC_MPRV         (CC_BASE | 0x05)  // Previous track
#define CC_MPLY         (CC_BASE | 0x06)  // Play/Pause
#define CC_MSTP         (CC_BASE | 0x07)  // Stop

// ============================================================================
// Layer switching keys - Range 0x0200-0x02FF
// Prefix: LY_ (LaYer)
// ============================================================================
#define LY_BASE         0x0200
#define LY_MO1          (LY_BASE | 0x01)  // Momentary Layer 1
#define LY_MO2          (LY_BASE | 0x02)  // Momentary Layer 2

// ============================================================================
// Macro keys - Range 0x0300-0x03FF
// Prefix: MC_ (MaCro)
// ============================================================================
#define MC_BASE         0x0300
#define MC_COPY         (MC_BASE | 0x01)  // Ctrl+C
#define MC_PSTE         (MC_BASE | 0x02)  // Ctrl+V (Paste)
#define MC_CUT          (MC_BASE | 0x03)  // Ctrl+X
#define MC_UNDO         (MC_BASE | 0x04)  // Ctrl+Z
#define MC_REDO         (MC_BASE | 0x05)  // Ctrl+Y
#define MC_SAVE         (MC_BASE | 0x06)  // Ctrl+S
#define MC_FIND         (MC_BASE | 0x07)  // Ctrl+F
#define MC_SALL         (MC_BASE | 0x08)  // Ctrl+A (Select All)

// ============================================================================
// LED control keys - Range 0x0400-0x04FF
// Prefix: LD_ (LeD)
// ============================================================================
#define LD_BASE         0x0400
#define LD_BRIU         (LD_BASE | 0x01)  // Brightness up
#define LD_BRID         (LD_BASE | 0x02)  // Brightness down

// ============================================================================
// System/Firmware keys - Range 0x0500-0x05FF
// Prefix: SYS_ (SYStem)
// ============================================================================
#define SYS_BASE        0x0500
#define SYS_RST         (SYS_BASE | 0x01)  // Soft reset
#define SYS_BOOT        (SYS_BASE | 0x02)  // Enter bootloader

// ============================================================================
// Keycode type checking macros
// ============================================================================
#define IS_CONSUMER_KEY(kc)  (((kc) & 0xFF00) == CC_BASE)
#define IS_LAYER_KEY(kc)     (((kc) & 0xFF00) == LY_BASE)
#define IS_MACRO_KEY(kc)     (((kc) & 0xFF00) == MC_BASE)
#define IS_LED_KEY(kc)       (((kc) & 0xFF00) == LD_BASE)
#define IS_SYSTEM_KEY(kc)    (((kc) & 0xFF00) == SYS_BASE)
#define IS_BASIC_KEY(kc)     ((kc) <= 0xFF && (kc) > KC_TRNS)
#define IS_TRANSPARENT(kc)   ((kc) == KC_TRNS)
#define IS_MOD_KEY(kc)       ((kc) >= KC_LCTL && (kc) <= KC_RGUI)

// Extract layer number from LY_MO() keys
#define GET_LAYER(kc)        ((kc) & 0xFF)

// ============================================================================
// Convenience aliases for keymap readability
// ============================================================================
#define ______          KC_TRNS   // Transparent key
#define XXXXXXX         KC_NO     // Explicitly disabled key

#endif // KEYCODE_H
