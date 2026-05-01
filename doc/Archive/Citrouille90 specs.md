# Citrouille90 (AVRDU.Enc.Choc)

## 1. Overview

**Citrouille90 (AVRDU.Enc.Choc)** is a custom 90-key USB keyboard built around the **AVR64DU32** microcontroller.

Key characteristics:

* 90 keys (10 rows × 9 columns matrix, physically 5 rows × 20 columns)
* Kailh Choc low-profile switches
* One rotary encoder (PES12) without integrated push switch
* Dedicated hardware reset button (6mm × 6mm through-hole tact switch)
* USB Full-Speed device (keyboard + bootloader)
* Amber LEDs (1 power LED + 2 PWM indicator LEDs)
* Minimalist PCB, minimal 3D-printed sandwich case
* Firmware-driven bootloader entry (software reset via key combination)
* Hardware reset via dedicated tact switch
* Toolchain: avr-gcc compiler, UPDI flashing software = avrdude with serialupdi, USB flashing software custom (could be simple python script)

In general we want to borrow from QMK, DxCore, MCC, LUFA, TinyUSB the ideas that still apply well to the AVR-DU, and enhance or adapt where it makes sense.

---

## 2. MCU Selection

**Microcontroller:** AVR64DU32

Reasons:

* Native USB Full-Speed (no external USB PHY)
* UPDI programming and recovery
* Supports USB HID natively
* 32 pins total: 23 usable GPIO

Specifications:

* 24 MHz clock
* 64 KB Flash
* 8 KB SRAM

---

## 3. Power Configuration

* USB bus power (5V from host)
* Datasheet **Power Configuration 5b**
* VBUS → VDD (5V domain)
* Internal regulator provides VUSB (3.3V USB core)

Decoupling:

* 4.7µF ceramic bulk capacitor on VBUS (near USB connector, X7R/X5R, 16V, after polyfuse)
* Local decoupling near MCU pins:
  * 2× 100nF ceramic at VDD pins (pins 18, 28)
  * 1× 100nF ceramic at VUSB pin (pin 6)
* Total capacitance: 4.7µF + 0.5µF = 5.2µF (safely under USB 10µF maximum)
* VUSB pin decoupled to ground only (no external load)

---

## 4. USB Interface

* USB Full-Speed device
* USB Mini-B connector (through-hole)

Bootloader (KeyDU.BL) exposes USB interface (Vendor defined class). 
Keyboard app (Citrouille.KBapp) exposes USB HID interface.

### 4.1 USB Device Modes

**Bootloader Mode:**

* Vendor-defined class
* Single interface for firmware updates, control transfer
* Requires LibUSB on PC with python script
* consult https://github.com/microchip-pic-avr-examples/avr64du32-cnano-usb-vendor-mplab-mcc for ideas and change approach to control transfer 

**Application Mode (Keyboard Firmware):**

* Composite USB HID device
* Three interfaces (see section 4.2)

### 4.2 HID Interface Design

Citrouille90 implements a **composite USB HID device** with three interfaces to provide maximum compatibility and functionality:

#### Interface 0: Boot Keyboard (HID Page 0x07)

* **bInterfaceClass:** 0x03 (HID)
* **bInterfaceSubClass:** 0x01 (Boot Interface)
* **bInterfaceProtocol:** 0x01 (Keyboard)
* **UEFI/BIOS Compatible:** ✅ Yes
* **Endpoint:** IN (keyboard reports)
* **Coverage:** All standard keyboard keys
  * Alphanumeric (A-Z, 0-9)
  * Function keys (F1-F24)
  * Modifiers (Ctrl, Shift, Alt, Win)
  * Navigation (arrows, Home, End, PgUp, PgDn)
  * Editing (Insert, Delete, Backspace)
  * Keypad (all keys)
  * International keys
  * VK_POWER via HID 0x07:0x66 (Keyboard Power)
* **Implementation:** 6KRO (6-key rollover) standard boot protocol

#### Interface 1: Boot Mouse (HID Page 0x01 - Generic Desktop)

* **bInterfaceClass:** 0x03 (HID)
* **bInterfaceSubClass:** 0x01 (Boot Interface)
* **bInterfaceProtocol:** 0x02 (Mouse)
* **UEFI/BIOS Compatible:** ✅ Yes
* **Endpoint:** IN (mouse reports)
* **Coverage:** 
  * X/Y movement (relative)
  * 3 buttons (Left, Right, Middle)
  * Vertical scroll wheel
* **Implementation:** Standard 5-byte boot mouse protocol
* **Use cases:**
  * Encoder can control scroll wheel (works in UEFI and OS)
  * Keys can trigger mouse buttons (via layer switching)
  * UEFI menu navigation via mouse

#### Interface 2: Extended Controls (HID Page 0x0C + 0x01)

* **bInterfaceClass:** 0x03 (HID)
* **bInterfaceSubClass:** 0x00 (No Subclass)
* **bInterfaceProtocol:** 0x00 (None)
* **UEFI/BIOS Compatible:** ❌ No (OS only)
* **Endpoint:** IN (extended control reports)
* **Coverage:**
  * **Consumer Controls (HID Page 0x0C):**
    * Volume (Up, Down, Mute)
    * Media (Play/Pause, Stop, Next Track, Previous Track)
    * Browser (Home, Back, Forward, Refresh, Search, Favorites, Stop)
    * System (Mail, Media Select, Calculator, My Computer)
  * **System Controls (HID Page 0x01):**
    * VK_SLEEP (System Sleep - HID 0x01:0x82)
* **Implementation:** Combined report descriptor with 16-bit consumer codes + 1-bit system sleep control

### 4.3 Endpoint Allocation

```
Endpoint 0:    Control (bidirectional) - USB enumeration, configuration
Endpoint 1 IN: Keyboard reports (Interface 0)
Endpoint 2 IN: Mouse reports (Interface 1)
Endpoint 3 IN: Extended control reports (Interface 2)
```

Total: 3 IN endpoints + 1 control endpoint (well within AVR64DU32 capabilities)

### 4.4 UEFI/BIOS Compatibility

When booting or in UEFI/BIOS:

* ✅ **Interface 0 (Keyboard)** - Fully functional for typing and navigation
* ✅ **Interface 1 (Mouse)** - Fully functional for cursor control and clicking
* ❌ **Interface 2 (Extended Controls)** - Ignored (no media keys in UEFI - expected behavior)

Once OS loads:

* ✅ All three interfaces functional
* ✅ Full media key support
* ✅ Full VK code coverage from reference table

### 4.5 Report Descriptor Strategy

**Interface 0 - Boot Keyboard:**

* Standard 8-byte boot keyboard report
* 1 byte modifiers + 1 byte reserved + 6 bytes keycodes (6KRO)

**Interface 1 - Boot Mouse:**

* Standard 5-byte boot mouse report
* Buttons + X delta + Y delta + Wheel delta + Horizontal wheel (optional)

**Interface 2 - Extended Controls:**

```c
Report structure (4 bytes):
  Byte 0: Report ID (0x03)
  Byte 1-2: Consumer code (16-bit, 0x0000 = none)
  Byte 3 bit 0: System Sleep (1 = pressed, 0 = released)
  Byte 3 bits 1-7: Padding (reserved)
```

### 4.6 VK Code Coverage Summary-

This three-interface design provides **complete coverage** of all VK codes from the reference table:

* **HID Page 0x07 (Keyboard)** → Interface 0
* **HID Page 0x0C (Consumer)** → Interface 2
* **HID Page 0x01 (System Sleep)** → Interface 2
* **HID Page 0x01 (Mouse)** → Interface 1

No VK codes are left unmapped.

### ### 4.7 Integrate Num Lock and Caps Lock led indicators

Add the EP OUT to **Interface 0 (Boot Keyboard)**.

The HID LED output report (Num Lock, Caps Lock, Scroll Lock, Compose, Kana) is part of the standard boot keyboard protocol. The host sends LED state via a **HID Output Report** on the keyboard interface, and BIOS/UEFI also knows to use Interface 0 for this since it's the boot-class interface.

The endpoint allocation would become:

```
Endpoint 0:    Control (bidirectional)
Endpoint 1 IN: Keyboard reports (Interface 0)
Endpoint 1 OUT: LED output reports (Interface 0)  ← add this
Endpoint 2 IN: Mouse reports (Interface 1)
Endpoint 3 IN: Extended control reports (Interface 2)
```

Note that EP1 OUT shares the endpoint number with EP1 IN — IN and OUT are separate pipes on the same endpoint number, which is normal and saves you an endpoint number.

**The output report format** (1 byte, no Report ID since Interface 0 has no report IDs):

```
Bit 0: Num Lock
Bit 1: Caps Lock
Bit 2: Scroll Lock
Bit 3: Compose
Bit 4: Kana
Bits 5-7: Padding
```

The host can deliver this either via the EP1 OUT endpoint or via a `SET_REPORT` control transfer on EP0 — you should support both. Many OSes use `SET_REPORT` (EP0) rather than the interrupt OUT endpoint, so if you want to keep things minimal you could skip the physical OUT endpoint and rely solely on `SET_REPORT`, but declaring the OUT endpoint in your descriptor is the more complete and correct implementation.

---

## 5. Key Matrix

* **10 rows × 9 columns = 90 keys**
* Kailh Choc low-profile switches
* Diode per switch
* Column output low to row input pull-up scanning
* matrix scan rate: 1000 Hz, or once every 1 ms

### Matrix GPIO allocation

Grouping rationale:

* Ports grouped to reduce register access
* Faster scanning
* Cleaner routing
* Lower firmware complexity

---

## 6. Rotary Encoder

* **Model:** PES12 (without integrated push switch)

* **Signals:**
  
  * Encoder A → Pnx
  * Encoder B → Pnx

* **No push switch in encoder** - separate reset functionality provided by dedicated tact switch

Encoder signals are handled in firmware (quadrature decoding).

---

## 7. Reset & Bootloader Strategy

### 7.1 Reset Methods Overview

Three reset methods available (one hardware, two software):

**Hardware Reset:**

* Physical tact switch on PCB
* Emergency recovery method
* Always functional regardless of firmware state

**Software Bootloader Reset:**

* Key combination enters bootloader (flashing mode)
* Primary method for firmware updates
* Convenient, no physical access required

**Software Plain Reset:**

* Key combination restarts keyboard
* Quick restart without entering bootloader

### 7.2 Hardware Reset

* **6mm × 6mm through-hole tact switch**
* Mounted on PCB (corner top right)
* Wired directly to **RESET pin** (pin 26) with switch series resistor (330 ohm) and 0.1uF cap as per datasheet
* Provides emergency recovery if firmware hangs or software reset fails
* Always functional regardless of firmware state
* silkscreen: "RESET"

### 7.3 Software Resets

**Two software reset keycodes available on Layer 2:**

**Bootloader Reset (for firmware flashing):**

* **Key combination**: Layer 0 → Layer 1 → Layer 2 → Bootloader keycode
* Firmware writes magic value to GPR register (r2, optionally r3 for additional security)
* Firmware triggers **watchdog reset**
* Bootloader checks GPR on startup
* If magic matches → stay in bootloader
* Otherwise → jump to application
* Bootloader clears register immediately after detection
* This is the primary/convenient method for entering bootloader mode for firmware updates

**Plain Reset:**

* **Key combination**: Layer 0 → Layer 1 → Layer 2 → Reset keycode
* Firmware triggers **watchdog reset** immediately
* No magic value written to GPR
* System resets and boots directly to application firmware
* Useful for quick keyboard restart without entering bootloader

### 7.4 Bootloader Request Mechanism

* Uses **General Purpose Register (GPR)** to signal bootloader entry
* Magic value written to GPR register r2 (optionally r3 with complement for additional security)
* Works only with watchdog reset (not power-on or BOD resets)
* GPR registers preserved across watchdog reset
* Registers undefined after power-on reset (low false positive probability ~1/256, or ~1/65536 with dual-register checking)

### 7.5 Bootloader Flow

1. Keyboard firmware detects bootloader key combo
2. Magic value written to reserved GPR (r2, optionally r3)
3. Watchdog reset triggered
4. Bootloader starts
5. Bootloader checks GPR very early (before C runtime init, .data, .bss clearing)
6. If magic matches → stay in bootloader
7. Otherwise → jump to application
8. Bootloader clears register immediately after detection

---

## 8. LEDs

### 8.1 Power LED

* Amber, through-hole (3mm diffused)
* Connected to **VDD (5V)**
* Fixed resistor (no GPIO, no PWM)
* Resistor value chosen for 1.4 mA, hence 2200 ohm
* Target: dim but non annoying in very dim environment, still visible (though dim) in brightly lit room
* Resistor placed close to LED (not MCU) to reduce EMI and simplify rework

### 8.2 Indicator LEDs (2×)

* Amber, through-hole, diffused (3mm)
* Driven by MCU GPIO with **PWM**
* Firmware-controlled brightness
* Target current: ~7.5 mA per LED at 100% duty cycle (visible in brightly lit room)
* 390 ohm resistor for each LED
* ~60–100° viewing angle preferred (soft glow, not sharp bright spots)

GPIO assignment:

* Indicator LED A PWM pin
* Indicator LED B PWM pin

Brightness control:

* Default brightness (PWM settings) stored in flash
* Runtime brightness stored in SRAM
* Layer 3 provides macros to adjust brightness
* Brightness range clamped: Min ~5–10%, Max 100%

PWM implementation:

* Use TCA timer (can drive both LEDs)
* PWM frequency: 1,465 Hz

```c
// New (1.465 kHz):
TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc;  // Prescaler 64
// f_PWM = 24 MHz / (64 × 256) = 1,465 Hz
```

---

## 9. GPIO Usage Summary

Total usable GPIOs: **23**

| Function       | Pins   |
| -------------- | ------ |
| Matrix rows    | 10     |
| Matrix columns | 9      |
| Encoder A/B    | 2      |
| PWM LEDs       | 2      |
| **Total**      | **23** |

RESET and UPDI pins are not counted as GPIO (dedicated functions).

---

## 10. Programming & Recovery

* **Primary flashing:** KeyDU.BL USB bootloader (via custom tool using LibUSB)
* **Recovery flashing:** UPDI

UPDI header:

* Placed at PCB edge (top edge preferred)
* Mandatory for unbrick / development
* 3 pins: GND, UPDI, VCC (see AVR-DU datasheet for pinout)
* **Never disable UPDI in fuses**

---

## 11. Bootloader Size Expectations

* ~2–6 kB
* Reserve 8 kB

---

## 12. Physical Layout Notes

* USB connector on top edge, aligned with middle of function key row
* Encoder located top-left
* Three LEDs arranged horizontally near top edge offset to the right
* Hardware reset via dedicated tact switch
* UPDI header at PCB edge
* 2-layer PCB
* 3D-printed case
* Low-profile design due to Kailh Choc switches

---

## 13. Branding & Logo

* Name: **Citrouille90**
* Trim: **(AVRDU.Enc.Choc)**
* Logo: "Citrouille90" text in **Playpen Sans font** combined with waveform graphic and pumpkin 

---

## 14. Firmware Architecture

### 14.1 Layers & Features

* 3 layers:
  * Layer 0: Base layer (default)
  * Layer 1: Function layer (accessed via Fn key on Layer 0)
  * Layer 2: System layer (accessed via key on Layer 1), includes bootloader entry, reset, and  LED brightness adjustment
* Support for small macros (e.g., Ctrl+C, Ctrl+V, Alt-Tab sequences)
* 6KRO (6-key rollover) USB HID

### 14.2 Development Approach

* Bare-metal approach + USB stack
* avr-gcc toolchain
* Direct register access with light HAL
* Need to write startup code + linker script (reference DxCore or Microchip MCC templates)
* Small hardware abstraction layer (HAL) for GPIO, timers, matrix scanning
* LUFA and MCC code provide examples for USB enumeration and HID keyboard reports

### 14.3 USB HID Descriptor

* Setup USB HID descriptor for standard keyboard
* 6KRO implementation (boot protocol compatible)
* Device strings: "Citrouille90" or similar

### 14.4 Memory Layout

Typical pattern:

```
|----------------------|  Flash start (0x0000)
| Bootloader           |
| (8 KB reserved)      |
|----------------------|  0x2000
| Application firmware |
| (keyboard logic)     |
|----------------------|  Flash end
```

Bootloader:

* Own USB stack (vendor class)
* Minimal functionality
* Occupies first 8 KB of flash

Application firmware:

* Full HID stack
* Keyboard logic
* Starts at 0x2000

### 14.5 Data Storage Strategy

**Flash (persistent, read-only at runtime):**

* Default keymaps (3 layers)
* Macro definitions
* USB descriptors
* Default LED brightness values

**SRAM (volatile, read-write):**

* Active layer state
* Matrix state
* Debounce counters
* Macro execution state
* Runtime LED brightness

**Design principle:**

* Flash: "What the keyboard is" (static configuration)
* SRAM: "What the keyboard is doing right now" (runtime state)

Keymaps stored strictly in flash (no EEPROM usage for keymaps).

On AVR Dx/DU:

* Flash is memory-mapped
* Can read it like normal const data (no special flash access API needed for reads)

---

## 15. Encoder Implementation

### 15.1 Encoder Hardware

* PES12 rotary encoder (no integrated push switch)
* Quadrature outputs: A, B
* Mechanical encoder with (optionally) tactile detents

### 15.2 Encoder Decoding Logic

* Quadrature decoder using state machine (recommended)
* State table-driven or bitwise decode
* Sample at matrix scan rate

### 15.3 Encoder Behavior Examples

* Volume control (CW: volume up, CCW: volume down)
* Alt-Tab with session logic:
  * Press Alt
  * Rotate encoder → tap Tab for each detent
  * Release Alt after pause (smart session handling)
* Mouse wheel emulation
* Brightness control
* Media timeline scrubbing

### 15.4 Encoder Design Pattern

Recommended approach (stored in flash):

```c
// Flash
const encoder_action_t encoder_map[] = {
  [ENCODER_1] = { .cw = MACRO_VOL_UP, .ccw = MACRO_VOL_DOWN },
};

// Flash
const macro_t macros[] = {
  MACRO_VOL_UP,
  MACRO_VOL_DOWN,
  MACRO_ALT_TAB,
  MACRO_ALT_SHIFT_TAB
};
```

Runtime: Track encoder deltas, dispatch macro IDs.

### 15.5 Encoder Considerations

* **Bounce:** Mechanical encoders bounce more than keys
  
  * Use state-table decoder (preferred), or
  * Light debouncing (~1–2 ms)

* **Alt-Tab behavior:** Some OSes require Alt held across multiple tabs
  
  * Implement encoder "session" logic (hold Alt until pause)

* **Power-on state:** Initialize encoder state early to avoid phantom rotations on boot

* **GPIO configuration:** Configure encoder pins with internal pull-ups enabled

### 15.6 Encoder Integration

* Scan encoders in same loop as matrix scanning
* Sample encoder pins each scan, decode transitions
* Encoders do not interfere with debouncing or layer logic

---

## 16. Component List Summary

**MCU:**

* 1× AVR64DU32 (32-pin TQFP)

**Switches:**

* 90× Kailh Choc low-profile switches
* 90× Diodes (1N4148W or similar, SOD-123)

**Encoder:**

* 1× PES12 rotary encoder (without push switch)
* Encoder knob

**Reset:**

* 1× 6mm × 6mm through-hole tact switch (SPST, normally open)

**USB:**

* 1× USB Mini-B connector (through-hole)
* 1× 500mA polyfuse (Bourns PTC RESET FUSE 15V 500MA 1812)
* 1× USB ESD protection IC (usblc6-2, SOT-23-6)

**Capacitors:**

* 1× 4.7µF ceramic capacitor (bulk, X7R, 16V, 1206)
* 5× 100nF ceramic capacitors (decoupling at MCU: 2× VDD, 1× VUSB, 1x ESD protection decoupling, 1x reset switch cap), 0805

**LEDs:**

* 3× 3mm through-hole amber LEDs (diffused)
  * 1× Power LED
  * 2× PWM indicator LEDs

**Resistors:**

* 1× Current limiting resistor for power LED (2.2kΩ)
* 2× Current limiting resistors for PWM LEDs (390Ω)
* 1x resistor for reset button (330 ohm)

**Programming:**

* 1× 3-pin UPDI header

**Keycaps:**

* 90× Kailh Choc-compatible keycaps (MBK, CFX, or similar profile)

**Case:**

* 3D-printed case (bottom & switch plate)
* Rubber feet

---

## 17. Next Steps

### 17.1 Immediate Tasks

1. **PCB layout**
   
   * Minimalist silkscreen with logo

2. **Design 3D-printed case**
   
   * Switch plate for Kailh Choc switches (2.2mm thickness)
   * Bottom case, use sandwitch mount
   * Encoder knob

3. **Firmware development**
   
   * Setup avr-gcc toolchain
   * Study DxCore and MCC examples for AVR-DU
   * Write startup code and linker scripts
   * Develop HAL layer
   * Create USB HID stack
   * Implement matrix scanning
   * Implement encoder decoding
   * Implement layer logic
   * Implement macro system
   * Implement LED PWM control

### 17.2 Design Decisions to Finalize

1. **Debouncing approach:**
   
   * Choose debounce method (time-based, state machine, etc.)
   * Set debounce timing (1–2 ms typical)

2. **Encoder implementation:**
   
   * Confirm quadrature decoder approach (state table recommended)
   * Finalize Alt-Tab session logic
   * GPIO pull-up configuration

### 17.3 Documentation to Create

* BOM.md (Bill of Materials with part numbers)
* PCBASSEMBLY.md (PCB assembly instructions)
* FirmwareBUILDING.md (Firmware build instructions)
* FLASHING.md (Bootloader entry and flashing guide)
* RECOVERY.md (UPDI unbrick procedure)
* CUSTOMIZATION.md (How to customize keymaps, macros, encoder behavior)

---

## 18. Research & Development Notes

### 18.1 Bootloader Details (KeyDU.BL)

* Vendor class bootloader
* Device enumerates as USB vendor class
* Requires LibUSB on PC
* Firmware update via custom tool (to be located in `hostusbflashing`)

### 18.2 UPDI Programming

* Never disable UPDI in fuses (critical for recovery)
* Use serialupdi programmer with avrdude
* Scripts in `hostupdiflashing` for bootloader and firmware flashing

---

## 19. License

* PCB and case: CERN-OHL-S v2 (Strongly Reciprocal)
* Art work: Creative Commons Attribution-ShareAlike 4.0 International License (CC BY-SA 4.0)
* Firmware: GNU General Public License version 3
* Copyright (C) 2026 Jonathan Ferron

https://certification.oshwa.org/basics.html
