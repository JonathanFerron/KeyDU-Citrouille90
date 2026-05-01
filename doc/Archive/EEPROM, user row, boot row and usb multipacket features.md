# Possible optional uses for some AVR-DU features

Do not integrate EEPROM, User Row or Boot Row in first version of firmware, but do consider the use of multipacket transfer if the benefit (time saved for firmware flashing) is significant.

---

## EEPROM (256 bytes)

The AVR64DU32 has 256 bytes of EEPROM, which is byte-erasable and survives power cycles and resets. For your keyboard, this is ideal for **user preferences that must persist but change at runtime** — things your current spec stores in flash (read-only) or SRAM (volatile):

- **Current layer remapping overrides** — if you ever want the user to swap a key permanently without reflashing
- **LED brightness setting** — currently you store defaults in flash and runtime state in SRAM; EEPROM lets the user's last-set brightness survive a power cycle
- **Encoder mapping selection** — e.g. "encoder is volume" vs "encoder is scroll" as a user toggle
- **Active keymap profile index** — if you add multiple profiles later
- **Debounce timing tuning** — per-unit calibration if you find switch variance

The key insight: EEPROM fills the gap between "compiled-in defaults in flash" and "lost-on-power SRAM state." Think of it as **the user's persistent preferences store**, distinct from the factory defaults in flash. At 256 bytes it's tight but more than enough for the above — a brightness byte, a profile index byte, a few flags, maybe 10–20 bytes of encoder config.

---

## User Row (64 bytes)

The User Row (USERROW) is a special 64-byte EEPROM-like region with its own address space, writable via UPDI or from firmware, and **not erased by a chip erase**. It survives even full flash reprogramming.

For your keyboard this is perfect for:

- **Keyboard serial number or unit ID** — useful if you ever build more than one unit or share the design
- **Manufacturing calibration data** — e.g. a tested LED resistor trim value per unit
- **Hardware revision tag** — so firmware can adapt if you do a PCB rev
- **Permanent owner config** — things the user sets once and never wants wiped even during full reflash (e.g. "this unit always boots to Dvorak")
- **Bootloader configuration flags** — e.g. a flag your bootloader reads to decide timeout behavior, without touching application flash

The critical property is **chip-erase survival**. When you flash new firmware via your KeyDU.BL bootloader or UPDI, the User Row stays intact. This makes it appropriate for identity and calibration data, not for things that should reset with firmware.

---

## Boot Row (512 bytes)

The Boot Row is a separately-protected 512-byte flash section accessible only from the bootloader (BOOTCODE) memory section, not from application code. It's a newer AVR-DU specific feature.

For your design the most compelling use is:

- **Bootloader configuration and state storage** — your KeyDU.BL bootloader could store its own version, feature flags, or update counters here, inaccessible to and unmodifiable by application firmware
- **Cryptographic anchor / firmware validation** — if you later want the bootloader to verify the application (a simple CRC or magic pattern stored in Boot Row that the bootloader checks before jumping)
- **Bootloader self-update metadata** — if you ever want KeyDU.BL to be field-updatable, the Boot Row can hold the update state machine's persistent data
- **Fallback keymap for bootloader mode** — your bootloader currently uses a key combo to enter flash mode; a minimal keymap in Boot Row lets the bootloader do its own independent scanning without depending on application data

The isolation is the value here: application firmware cannot corrupt Boot Row data, giving your bootloader a trustworthy private scratchpad.

---

## USB Multipacket Transfer

The AVR64DU32 USB peripheral supports multipacket (multi-packet) transfers on endpoints, meaning a single transaction can span multiple data packets handled in hardware before the CPU is interrupted. This is relevant in two places for your design:

**In the bootloader (KeyDU.BL):** Firmware upload is the primary use case. Without multipacket, your bootloader receives one 64-byte USB packet, interrupts the CPU, writes it to flash, then acknowledges, repeating ~1000 times for a 64KB image. With multipacket, the USB hardware can buffer several packets and interrupt the CPU less frequently, making your flashing tool faster and the code simpler. For a 64KB firmware upload this can meaningfully reduce flash time and simplify your Python flashing script's timing requirements.

**In the application (HID reports):** For your three HID interfaces this matters less since keyboard/mouse reports are small (8 bytes, 5 bytes, 4 bytes). However, if you later add a raw HID interface for configuration (a common QMK pattern — letting software on the PC read/write keymaps at runtime), multipacket becomes useful for transferring a full keymap layer in one logical transaction rather than many small ones.

The practical advice: design your bootloader's OUT endpoint to take advantage of multipacket from the start, since that's where you'll feel the benefit most concretely during development iterations when you're reflashing constantly.

---

## Summary Table

| Feature | Size | Best use in Citrouille90 |
|---|---|---|
| EEPROM | 256 B | User runtime preferences (brightness, profile, encoder mode) |
| User Row | 64 B | Unit identity, permanent calibration, bootloader flags |
| Boot Row | 512 B | Bootloader-private config, firmware validation anchor |
| Multipacket | — | Fast firmware upload in bootloader, future raw HID config |

The clean design principle that falls out of this: your spec's current flash/SRAM split is right for static vs runtime data, but adding EEPROM for **persistent user preferences** and User Row for **permanent unit identity** gives you a complete four-tier memory strategy that matches what every serious keyboard firmware (QMK included) ends up needing once users start customizing their boards.