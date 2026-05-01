Based on the Citrouille90 specs, here are QMK features worth studying for your firmware design:

- usb enumeration, matrix scanning code, debouncing code, rotary encoder code.

## Core Architecture Features

**Layer Management System**
- QMK's layer precedence and activation logic (momentary, toggle, one-shot)
- Layer state tracking in a compact bitmap
- How layers are indexed and keycodes resolved through the stack

**Keycode System**
- QMK's keycode architecture separating basic keys, modifiers, layer keys, and special functions
- The `KC_*` macro system and how custom keycodes extend it
- Transparent keys (`KC_TRNS`) that fall through to lower layers

## Macro & Special Functions

**Macro Implementation**
- QMK's `SEND_STRING()` and `process_record_user()` pattern
- Macro playback state machines (character-by-character sending)
- Timing between macro keystrokes

**Tap/Hold Behaviors**
- Mod-Tap (key acts as modifier when held, regular key when tapped)
- Layer-Tap (similar but for layers)
- Could be useful for your Fn key implementation

## LED & Visual Feedback

**LED Indicators**
- Caps Lock, Num Lock layer indicators
- How QMK maps LED states to layer changes
- Brightness ramping/fading algorithms for smooth transitions

## Encoder Integration

**Encoder Layer Mapping**
- QMK's `encoder_update_user()` per-layer dispatch
- How encoder behavior changes with active layer
- Encoder resolution and acceleration curves

## Power Management

**USB Suspend/Resume**
- How QMK handles USB suspend requests
- Wakeup key detection
- Power state restoration after sleep

## Configuration System

**Compile-Time Configuration**
- QMK's `config.h` and `rules.mk` pattern
- Feature flags (`MOUSEKEY_ENABLE`, `EXTRAKEY_ENABLE`, etc.)
- How this enables/disables code sections to save flash

**Keymap Definition**
- The `LAYOUT()` macro system
- How keymaps are stored in flash as 2D arrays
- Keycode to HID scancode translation tables

## Timing & Scheduling

**Deferred Execution**
- QMK's `defer_exec()` for delayed actions
- Useful for your Alt-Tab "session" logic (releasing Alt after pause)
- Timeout-based state transitions

**Matrix Scan Hook**
- `matrix_scan_user()` for per-scan-cycle logic
- Where to integrate encoder scanning
- Managing scan timing budget

## HID Report Building

**Report Construction**
- How QMK builds the 8-byte keyboard report from active keys
- Modifier byte packing
- Handling 6KRO limitations (which keys to drop when >6 pressed)

**Media Keys & Consumer Control**
- Separate report for consumer/media keys
- Report ID management for composite devices
- Your Interface 2 (extended controls) maps directly to this

## EEPROM/Config Storage

While your spec says "no EEPROM for keymaps," QMK's EEPROM usage is still worth understanding:
- Magic number validation
- Configuration versioning
- Default value restoration
- You might want runtime brightness settings to persist (though you could use UserRow flash instead)

## Debug & Development Features

**Console Output**
- QMK's `print()` and debug console over USB
- Very useful during firmware development
- Consider implementing a simple debug interface in your bootloader mode

## What NOT to Port

- **VIA/VIAL** dynamic keymap runtime editing (overcomplicated for your needs)
- **RGB matrix effects** (you only have 2 amber PWM LEDs)
- **Audio/speaker support** (not in your spec)
- **Split keyboard support** (single board design)
- **Bluetooth/wireless** (USB only)

## Study Priority

1. **High priority**: Layer system, keycode architecture, macro implementation, LED indicators
2. **Medium priority**: Encoder layer mapping, tap/hold, USB suspend/resume, timing system
3. **Low priority**: EEPROM patterns (for reference), debug console (development tool)

The QMK codebase is large, but focusing on `quantum/quantum.c`, `quantum/keymap_common.c`, and `quantum/process_keycode/` subdirectory will give you the patterns you need without drowning in features you don't want.