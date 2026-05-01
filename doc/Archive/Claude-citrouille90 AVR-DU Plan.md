# citrouille90 AVR-DU Plan

**Created:** 12/25/2025 18:43:38  
**Updated:** 3/5/2026 20:42:34  
**Exported:** 3/28/2026 8:49:09  
**Link:** [https://claude.ai/chat/35f43ff4-c5d2-4a04-bea1-8ba5e3c7ac93](https://claude.ai/chat/35f43ff4-c5d2-4a04-bea1-8ba5e3c7ac93)  

## Prompt:
12/25/2025, 6:43:41 PM

> File: AmberClick90.md

please reorganize this document while not losing any information nor details. this is to save me the time to do this myself manually.



## Response:
12/25/2025, 6:46:26 PM

# AmberClick90 (AVRDU.Enc.Mat) - Technical Specification

## 1. Project Overview

**AmberClick90** is a custom 90-key USB keyboard featuring:

- **Physical Layout**: 90 keys arranged in a 10×9 matrix (physically 5 rows × 18 columns)
- **Switches**: Matias Click mechanical switches
- **Encoder**: One ALPS EC11 rotary encoder with integrated push switch
- **Connectivity**: USB Full-Speed via USB Mini-B connector
- **Indicators**: 3 amber LEDs (1 power, 2 PWM-controlled indicators)
- **Aesthetics**: Minimalist black PCB with minimal grey 3D-printed case
- **Programming**: UPDI for initial/recovery flashing, USB HID bootloader for updates

---

## 2. Hardware Specifications

### 2.1 Microcontroller: AVR64DU32

**Key Specifications:**
- 64 KB Flash memory
- 8 KB SRAM
- 24 MHz internal oscillator
- 32-pin package with 23 usable GPIO pins
- Native USB Full-Speed (no external PHY required)
- UPDI programming interface
- Internal USB regulator (3.3V)
- Operates at 5V input (from USB)

**Advantages over ATmega32U4:**
- 2× flash capacity
- 3× SRAM capacity
- Better USB implementation
- No external crystal required
- UPDI instead of ISP programming
- More GPIO pins (23 vs ~23 on 32U4, but cleaner power design)

### 2.2 Power Configuration

**Design**: USB Bus Powered (Datasheet Power Configuration 5b)

- VBUS → VDD (5V domain from USB host)
- Internal regulator provides VUSB (3.3V for USB core)

**Decoupling Requirements:**
- Bulk capacitor on VDD near USB connector (≤10µF per USB specification)
- Local decoupling capacitors near all MCU VDD pins
- VUSB pin decoupled to ground only (no external load)

### 2.3 GPIO Pin Allocation

**Total Usable GPIO: 23 pins**

| Function | Pin Assignment | Count |
|----------|----------------|-------|
| **Matrix Columns (9)** | PA0–PA3, PD1–PD5 | 9 |
| **Matrix Rows (10)** | PC3, PD0, PD6, PD7, PF0–PF5 | 10 |
| **Encoder A/B** | PA6, PA7 | 2 |
| **PWM Indicator LEDs** | PA4 (LED A), PA5 (LED B) | 2 |
| **Total** | | **23** |

**Pin Grouping Rationale:**
- Ports grouped to minimize register access during scanning
- Faster matrix scanning performance
- Cleaner PCB routing
- Reduced firmware complexity

**Reserved Pins (not counted as GPIO):**
- RESET pin (connected to encoder push switch)
- UPDI pin (programming interface)

### 2.4 Key Matrix

**Configuration**: 10 rows × 9 columns = 90 keys

**Implementation:**
- Matias Click switches
- One diode per switch (1N4148 or equivalent)
- Standard keyboard matrix configuration

**Scanning Method:**
- Column-driven or row-driven (TBD during firmware development)
- Debouncing in firmware (~1-2ms typical)
- Scan rate: ~1-2 kHz typical

### 2.5 Rotary Encoder

**Model**: ALPS EC11 with detent clicks

**Connections:**
- Encoder Phase A → PA6
- Encoder Phase B → PA7
- Integrated push switch → RESET pin (hardware reset only)

**Firmware Handling:**
- Quadrature decoding in main scan loop
- State machine or table-driven decoder
- Light debouncing (1-2ms) due to mechanical bounce
- Can implement "smart" Alt-Tab behavior (hold Alt across multiple Tab presses)

**Encoder Actions (Example):**
- Clockwise: Volume up / Alt+Tab
- Counter-clockwise: Volume down / Alt+Shift+Tab
- Can be mapped to macros stored in flash

### 2.6 LED Configuration

#### Power LED (Non-PWM)
- **Type**: 3mm through-hole amber LED, diffused
- **Connection**: VDD (5V) with current-limiting resistor
- **Current**: 0.2-0.8mA (typically 0.3-0.5mA)
- **Brightness**: Fixed, intentionally dim
- **Design Method**: Test resistor value in very dim environment to ensure non-annoying brightness while remaining visible in bright indoor lighting

#### Indicator LEDs (2× PWM-Controlled)
- **Type**: 3mm through-hole amber LEDs, diffused
- **Connections**: 
  - LED A → PA4 (PWM capable)
  - LED B → PA5 (PWM capable)
- **Target Current**: ~5mA per LED at 100% duty cycle
- **Forward Voltage**: ~2.0V (typical amber)
- **Resistor Calculation**: R ≈ (3.3V - 2.0V) / 0.005A ≈ 270Ω

**PWM Configuration:**
- Frequency: ≥500 Hz, preferably 1-2 kHz
- Resolution: 8-bit (0-255)
- Default brightness stored in Flash
- Runtime brightness stored in SRAM
- Brightness range: 5-10% minimum, 100% maximum

**Brightness Control:**
```c
#define LED_BRIGHTNESS_DEFAULT 128  // Flash (compile-time)
uint8_t led_brightness;              // SRAM (runtime)
```

**LED Advantages (3mm Diffused):**
- Smaller emitting area reduces glare
- Diffusion hides PWM artifacts
- Softer appearance in low light
- 60-100° viewing angle ideal for indicators

### 2.7 USB Interface

**Connector**: USB Mini-B, through-hole

**Design Requirements:**
- Route D+/D- as short, matched-length differential pair
- Minimize stubs and avoid vias if possible
- Series resistors on D+ and D- lines (typically 22-33Ω)
- Internal USB pull-up (if MCU provides) or external 1.5kΩ on D+
- USB bulk capacitor (≤10µF total per USB spec)
- Optional: Low-capacitance USB ESD protection diode array
- Optional: 500mA polyfuse near USB connector

**USB Operation:**
- No external oscillator required
- Internal 24MHz oscillator with USB clock recovery (SOF-based)
- USB Full-Speed compliant

### 2.8 Programming Interfaces

#### UPDI Header
- **Location**: PCB edge (top edge preferred for case design)
- **Pins**: 3 or 4 pins (consult AVR64DU32 datasheet for exact pinout)
- **Purpose**: 
  - Initial firmware programming
  - Recovery/unbrick capability
  - Development/debugging
- **Critical**: Never disable UPDI via fuses

#### USB Bootloader (HID)
- **Type**: HID bootloader (keyboard-class device)
- **Entry Method**: Firmware key combination (Fn + LeftCtrl + RightCtrl)
- **Advantages**:
  - No drivers required on any OS
  - Same device class as keyboard
  - Works on locked-down systems
  - QMK-compatible workflow

---

## 3. Firmware Architecture

### 3.1 Development Toolchain

**Core Tools:**
- **Compiler**: avr-gcc
- **Editor**: Geany
- **UPDI Programmer**: SerialUPDI or similar (research required)
- **USB Flashing**: Custom HID tool (research QMK-style tools)

**Architecture**: Bare-metal + TinyUSB + minimal HAL

### 3.2 Firmware Structure

**Typical HAL Organization:**
```
/src
  main.c
  usb_descriptors.c
  keyboard.c
  matrix.c
  encoder.c
  hal_gpio.c
  hal_timer.c
  config.h
  bootloader.c
```

### 3.3 Memory Layout

#### Flash (64 KB total)
```
|------------------------| 0x0000
| HID Bootloader         |
| (4-6 KB, ~6-9%)        |
|------------------------|
| Application Firmware   |
| (18-22 KB estimated)   |
| - Keyboard logic       |
| - Matrix scanning      |
| - Layer engine         |
| - Macro engine         |
| - TinyUSB stack        |
|------------------------|
| Unused                 |
| (36-42 KB free)        |
|------------------------| 0xFFFF
```

#### Flash Usage Breakdown (Application Only)

| Component | Size |
|-----------|------|
| Startup + HAL (GPIO, timer) | 1.5-2 KB |
| Matrix scanning (90 keys) | 1-1.5 KB |
| Debouncing logic | 0.5-1 KB |
| Layer engine (2 layers) | 0.5 KB |
| Macro engine (simple) | 0.5-1 KB |
| Keymap data (2×90 keys) | ~360 bytes |
| Keyboard logic glue | ~1 KB |
| **Non-USB Subtotal** | **6-8 KB** |
| TinyUSB core | 8-10 KB |
| HID class (keyboard, NKRO) | 3-4 KB |
| USB descriptors | ~0.5 KB |
| **USB Subtotal** | **12-14 KB** |
| **Total Application** | **18-22 KB** |

#### SRAM (8 KB total)

| Component | Size |
|-----------|------|
| Key matrix state (90 keys) | ~90 bytes |
| Debounce buffers | ~180 bytes |
| Macro buffers | 64-128 bytes |
| USB HID reports | 64-128 bytes |
| TinyUSB buffers | 1-1.5 KB |
| Stack | ~1 KB |
| **Total Used** | **2.5-3.5 KB** |
| **Free** | **4-5 KB** |

### 3.4 Data Storage Strategy

**Flash (Read-Only at Runtime):**
- Default keymaps (2 layers × 90 keys)
- Macro definitions
- USB descriptors
- Default LED brightness values
- Encoder action mappings

**SRAM (Runtime State):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness
- Macro execution state
- Current encoder position

**Rationale:**
- Keymaps are constants defined at compile-time
- Flash reads are single-cycle on AVR DU
- No performance penalty for flash-based lookup tables
- Matrix scanning is much slower than flash access

### 3.5 Firmware Features

#### Layer System
- 2 main layers with real keycodes
- Optional 3rd layer for system functions:
  - LED brightness adjustment
  - Bootloader entry
  - System configuration

#### Macro Support
- Simple macros (key sequences)
- Example: One key → Ctrl+C
- Stored in flash
- Execution state in SRAM

#### NKRO (N-Key Rollover)
- Full NKRO HID descriptor (research required)
- All 90 keys can be pressed simultaneously
- USB HID report format to be defined

#### Encoder Features
- Quadrature decoding
- Macro dispatch on rotation
- Optional "smart" Alt-Tab (hold Alt across rotations)
- Configurable actions per layer

#### Debouncing
- Matrix: Standard keyboard debouncing (~5-20ms typical)
- Encoder: Light debouncing (1-2ms) due to increased mechanical bounce

### 3.6 USB Device Configuration

**USB Descriptors (TBD):**
- Device name: "AmberClick90"
- Manufacturer: [Your name/handle]
- Bootloader string: "AmberClick90 HID"
- Version tag: "AC90-r1"

**HID Report Descriptor:**
- NKRO keyboard format (research required)
- Separate descriptors for bootloader vs application

---

## 4. Bootloader Implementation

### 4.1 Bootloader Type: HID (Keyboard-Class)

**Specifications:**
- Flash size: 4-6 KB (~6-9% of total)
- SRAM: 300-600 bytes
- Enumeration: USB HID device
- Protocol: Custom (similar to QMK)
- No drivers required on host

**Advantages:**
- ✓ Driver-less operation on all OS
- ✓ Same device class as keyboard
- ✓ Works on restricted systems
- ✓ Easy integration with existing workflows

### 4.2 Bootloader Entry Mechanism

**Primary Method**: Firmware Key Combination
- **Combo**: Fn + LeftCtrl + RightCtrl
- **Detection**: During normal matrix scanning
- **Process**: Firmware-initiated reset into bootloader

**Secondary Method**: Manual Recovery
- **Method**: UPDI programming
- **Purpose**: Unbrick/development only

### 4.3 Bootloader Request Flow

#### Step 1: Key Combination Detection
```c
if (fn_layer_active &&
    key_is_pressed(KEY_LEFT_CTRL) &&
    key_is_pressed(KEY_RIGHT_CTRL)) {
    request_bootloader();
}
```

#### Step 2: Set Boot Flag (GPR Method)

**Magic Value Storage:**
- Use General Purpose Register (r2 or r3 recommended)
- Write magic value: `0x42`
- Survives watchdog reset only
- Does NOT survive power-on or brown-out reset

```c
#define BOOT_MAGIC 0x42

void request_bootloader(void) {
    asm volatile (
        "ldi r2, %0\n"
        :
        : "M" (BOOT_MAGIC)
    );
    wdt_enable(WDTO_15MS);
    while (1) {}  // Wait for watchdog reset
}
```

#### Step 3: Watchdog Reset
- Use watchdog timer (NOT software jump)
- Cleanest reset method
- Preserves GPR values

#### Step 4: Early Boot Check

**Must occur before:**
- .data initialization
- .bss clearing
- USB initialization
- Stack setup (ideally)

```c
// In bootloader startup (before C runtime init)
if (r2 == BOOT_MAGIC) {
    r2 = 0;  // Clear immediately
    stay_in_bootloader();
} else {
    jump_to_application();
}
```

### 4.4 Reset Behavior Summary

| Reset Type | GPR Preserved? | Use Case |
|------------|----------------|----------|
| Power-on | ❌ No | Normal startup |
| Brown-out | ❌ No | Voltage drop |
| Watchdog | ✅ Yes | Bootloader entry |
| External (RESET pin) | ❌ No | Manual reset |

### 4.5 Bootloader State Machine

**Boot Flow:**
```
Power On / Reset
    ↓
Bootloader Starts
    ↓
Check GPR for BOOT_MAGIC
    ↓
   [Magic Present?]
    ↓              ↓
   Yes            No
    ↓              ↓
Stay in         Jump to
Bootloader      Application
(USB HID)       (Keyboard)
```

**User-Visible Behavior:**

| Scenario | Result |
|----------|--------|
| Normal power-on | Keyboard starts immediately |
| Fn+Ctrl+Ctrl pressed | Keyboard → Bootloader mode |
| Encoder button (RESET) | Firmware restart |
| UPDI programming | Direct flash write |

### 4.6 Bootloader-Application Communication

**Fuse Configuration:**
- Reserve bootloader section in flash
- Protect bootloader from application overwrites
- Never disable UPDI

**Jump Mechanism:**
```c
#define BOOTLOADER_ADDR 0x0000  // Adjust per fuse settings

void jump_to_bootloader(void) {
    cli();  // Disable interrupts
    ((void (*)(void))BOOTLOADER_ADDR)();
}
```

### 4.7 USB Enumeration Behavior

**From Host Perspective:**

1. **Normal Operation**: Device enumerates as "AmberClick90" keyboard
2. **Bootloader Entry**: 
   - Keyboard disconnects
   - New HID device appears (bootloader)
3. **Firmware Update**: Host tool sends firmware via HID reports
4. **Completion**: Device resets, keyboard re-enumerates

---

## 5. Component Selection

### 5.1 Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **MCU** | AVR64DU32 | 1 | Use internal oscillator |
| **USB Connector** | USB Mini-B, through-hole | 1 | Full-Speed capable |
| **UPDI Header** | 3-4 pin header | 1 | See datasheet for pinout |
| **USB Series Resistors** | 22-33Ω, 0805 | 2 | For D+, D- |
| **USB Bulk Cap** | 10µF max, near connector | 1 | Per USB specification |
| **VDD Decoupling Caps** | 100nF, 0805 | Per VDD pin | Near each VDD pin |
| **VUSB Decoupling Cap** | 100nF, 0805 | 1 | Near VUSB pin |
| **Polyfuse** | 500mA, resettable | 1 | Optional, near USB |
| **ESD Protection** | Low-cap USB diode array | 1 | Optional |
| **Switches** | Matias Click | 90 | Mechanical switches |
| **Diodes** | 1N4148 or equivalent | 90 | One per switch |
| **Rotary Encoder** | ALPS EC11 with switch | 1 | With detent clicks |
| **Power LED** | 3mm amber, diffused, TH | 1 | Fixed brightness |
| **Indicator LEDs** | 3mm amber, diffused, TH | 2 | PWM-controlled |
| **LED Resistors** | TBD (see calculations) | 3 | Near LEDs |

### 5.2 LED Resistor Calculations

#### Power LED
- **Target**: Dim but visible (0.3-0.5mA typical)
- **Formula**: R = (VDD - Vf) / I = (5V - 2.0V) / 0.0005A ≈ 6kΩ
- **Selection Method**: Test in very dim environment to ensure non-annoying brightness

#### Indicator LEDs (PWM)
- **Target**: 5mA at 100% duty cycle (bright indoor visibility)
- **Formula**: R = (3.3V - 2.0V) / 0.005A = 260Ω
- **Standard Value**: 270Ω
- **Power**: P = I² × R = (0.005)² × 270 ≈ 7mW
- **Resistor Placement**: Near LED, not MCU (reduces EMI, easier rework)

---

## 6. PCB Design Guidelines

### 6.1 Physical Layout

**Component Placement:**
- USB connector: Top edge, aligned with Fn key
- Rotary encoder: Top-left corner
- Three LEDs: Horizontal arrangement near top edge
- UPDI header: Top edge (easier case design) or alternate edge for cleaner routing
- MCU: Central location to minimize trace lengths

**Silkscreen:**
- Project name: "AmberClick90"
- Trim designation: "(AVRDU.Enc.Mat)"
- Typographic logo design (TBD)
- Minimalist, monochrome aesthetic
- Technical/instrument-like appearance

### 6.2 Design Specifications

- **PCB**: 2-layer, black solder mask
- **Case**: Minimal, grey 3D-printed
- **Aesthetic**: Minimalist, technical

### 6.3 Net Naming Convention

**Rows:**
- ROW_0 through ROW_9

**Columns:**
- COL_0 through COL_8

**Encoder:**
- ENC_A, ENC_B
- RESET (encoder switch)

**LEDs:**
- LED_PWR (power LED)
- LED_IND_A, LED_IND_B (indicators)

**USB:**
- USB_DP, USB_DM
- VBUS, GND

**Programming:**
- UPDI
- RESET

### 6.4 Critical Routing

**USB Differential Pair:**
- Keep D+ and D- as short, matched-length traces
- Minimize stubs
- Avoid vias if possible
- Route as differential pair (typical impedance: 90Ω)

**Power:**
- Wide traces for VDD/GND
- Star-ground from USB connector
- Separate analog/digital grounds if needed

**Matrix:**
- Keep row/column traces away from USB lines
- Group by port for easier scanning

---

## 7. Firmware Development Roadmap

### 7.1 Initial Development Tasks

1. **Research Phase**
   - Study QMK ATmega32U4 implementation
   - Review DXCore for AVR DU specifics:
     - Pin mapping
     - Clock configuration
     - Fuse settings
     - Startup code
     - Linker scripts
     - UPDI flashing methods
   - Research TinyUSB integration
   - Define NKRO HID descriptor

2. **HAL Development**
   - GPIO abstraction (port-based access)
   - Timer setup for PWM
   - Watchdog configuration
   - Clock initialization

3. **Core Firmware**
   - Matrix scanning loop
   - Debouncing algorithm
   - Layer switching logic
   - Macro engine

4. **USB Stack**
   - TinyUSB integration
   - HID keyboard descriptor (NKRO)
   - USB enumeration
   - Device strings

5. **Bootloader**
   - HID bootloader implementation
   - GPR boot flag mechanism
   - Flash write routines
   - Application jump logic

### 7.2 Questions for LLM Assistant

**HAL & Architecture:**
- Sketch minimal HAL API for AVR64DU32
- Show TinyUSB HID report descriptor for NKRO
- Design HID bootloader strategy
- Provide bootloader entry mechanism (key combo vs reset)
- Show TinyUSB-compatible HID descriptor split (bootloader vs app)

**Matrix Scanning:**
- Show minimal scan loop for exact pin map (10 rows, 9 columns)
- Compare row-driven vs column-driven scanning
- Help decide pull-ups vs pull-downs
- Walk through diode orientation trade-offs

**Encoder Integration:**
- Sketch robust quadrature decoder
- Design Alt-Tab "smart encoder" behavior
- Suggest GPIO + pull-up configuration
- Show integration into matrix scan loop
- Show firmware logic for decoding encoders

**LED Control:**
- Suggest exact PWM timer setup for PA4, PA5
- Choose PWM frequency that won't alias with matrix scanning
- Show exact PWM timer config for AVR64DU32
- Recommend brightness step sizes
- Add cheap gamma correction
- Ensure PWM doesn't interfere with USB timing

**Power Analysis:**
- Check worst-case power consumption with everything on
- Estimate average current draw from USB
- Estimate maximum current draw (all LEDs + matrix)

**Bootloader Details:**
- Draft clean bootloader.c module
- Show minimal HID bootloader state machine
- Explain correct RESET button wiring (per datasheet)

**Clock & Fuses:**
- Research correct fuse settings (never disable UPDI)
- Select appropriate clock speed
- Evaluate sleep + wake-on-interrupt approach for power saving

---

## 8. Firmware Configuration Details

### 8.1 Clock Configuration

- **Primary Clock**: 24 MHz internal oscillator
- **USB Clock**: Derived from internal oscillator with USB clock recovery
- **Sleep Mode**: Evaluate wake-on-interrupt for power efficiency (research required)

### 8.2 Fuse Settings (Critical)

**Must Research:**
- Bootloader section size and location
- UPDI enable (NEVER disable)
- Brown-out detection level
- Watchdog configuration
- Reset pin configuration (tied to encoder switch)

### 8.3 Matrix Scanning Parameters

**Configuration:**
- Scan rate: 1-2 kHz typical
- Debounce time: 5-20ms (tune during testing)
- Pull configuration: TBD (pull-up vs pull-down)
- Diode orientation: TBD (COL2ROW vs ROW2COL)

**Port Optimization:**
- Rows grouped on single port where possible → single register read
- Columns distributed for routing convenience
- Minimize port switching during scan

### 8.4 PWM Configuration

**Timer Selection:**
- Use TCA or TCB for PWM
- Single timer can drive multiple PWM channels

**PWM Parameters:**
- Frequency: 1-2 kHz (avoid aliasing with matrix scan)
- Resolution: 8-bit (0-255)
- Brightness steps: 16-32 levels recommended
- Gamma correction: Optional, low-cost implementation

**Brightness Control:**
```c
// Default stored in Flash
#define LED_BRIGHTNESS_DEFAULT 128

// Runtime variable in SRAM
uint8_t led_brightness_a;
uint8_t led_brightness_b;

// Initialize at startup
led_brightness_a = LED_BRIGHTNESS_DEFAULT;
led_brightness_b = LED_BRIGHTNESS_DEFAULT;
```

### 8.5 Encoder Configuration

**Decoder Type:**
- State machine (preferred) or table-driven
- Handles mechanical bounce
- Integrates into main scan loop

**Sampling:**
- Poll during matrix scan (1-2 kHz)
- Or use pin-change interrupt (optional)

**Action Mapping (Example):**
```c
// Stored in Flash
const encoder_action_t encoder_map[] = {
    [LAYER_0] = { .cw = MACRO_VOL_UP,    .ccw = MACRO_VOL_DOWN },
    [LAYER_1] = { .cw = MACRO_ALT_TAB,   .ccw = MACRO_ALT_SHIFT_TAB }
};
```

---

## 9. Design Philosophy & Best Practices

### 9.1 Memory Management

**Flash Usage:**
- Store all constant data (keymaps, macros, descriptors)
- No runtime writes (except via bootloader)
- Maximize utilization of 64KB capacity

**SRAM Usage:**
- Minimize static allocation
- Use for runtime state only
- Monitor stack usage carefully

### 9.2 Code Quality

**Style:**
- Direct register access
- Minimal abstraction where performance critical
- Clean HAL for portability
- Well-commented for maintainability

**Portability:**
- Borrow concepts from QMK and DXCore
- Adapt for AVR DU specifics
- Document deviations from reference implementations

### 9.3 User Experience

**Boot Time:**
- Instant firmware start (<100ms)
- Quick bootloader check (no delay)

**Responsiveness:**
- Low-latency matrix scanning
- Immediate key feedback
- Smooth encoder operation

**Reliability:**
- Robust debouncing
- No false triggering
- Stable USB enumeration

---

## 10. Testing & Validation

### 10.1 Hardware Testing

- [ ] USB enumeration on multiple hosts (Windows, macOS, Linux)
- [ ] All 90 keys register correctly
- [ ] Encoder rotation and detents
- [ ] Reset button (encoder push)
- [ ] All three LEDs illuminate
- [ ] PWM dimming works correctly
- [ ] UPDI programming successful
- [ ] Bootloader entry via key combo

### 10.2 Firmware Testing

- [ ] Matrix scanning accuracy (no ghost keys)
- [ ] Debouncing effectiveness
- [ ] Layer switching
- [ ] Macro execution
- [ ] NKRO verification (6+ simultaneous keys)
- [ ] Encoder decoding accuracy
- [ ] LED brightness control
- [ ] Bootloader mode entry/exit
- [ ] Firmware update via USB

### 10.3 Power Testing

- [ ] USB current draw (idle)
- [ ] USB current draw (all keys pressed)
- [ ] USB current draw (all LEDs at 100%)
- [ ] Verify ≤500mA total (USB spec compliance)

---

## 11. Future Enhancements (Optional)

**Pin-Saving Techniques (if needed):**
- Share LEDs with matrix columns (LED active only when column driven)
- Encoder pins as matrix keys (careful decoding required)
- Note: Current design has sufficient GPIO, these are optional

**Advanced Features:**
- VIA/VIAL support for runtime keymap editing
- RGB underglow (if desired)
- Audio output (piezo buzzer)
- Additional encoders
- OLED display

---

## 12. References & Resources

**Datasheets:**
- AVR64DU32 datasheet (Microchip)
- ALPS EC11 encoder datasheet
- USB 2.0 specification

**Software:**
- QMK Firmware (reference implementation)
- DXCore (AVR DU support)
- TinyUSB (USB stack)
- avr-gcc documentation

**Standards:**
- USB HID Usage Tables
- USB 2.0 Full-Speed specification
- Keyboard scan code standards

---

## 13. Document Status

This specification represents the **current consolidated design** as of this revision and serves as:

- Complete reference for schematic and PCB design
- Firmware architecture guide
- Component selection reference
- Testing checklist
- Handoff document for future development

**Next Steps:**
1. Finalize typographic logo design
2. Create KiCad schematic
3. Complete PCB layout
4. Begin firmware HAL development
5. Implement and test bootloader

---

*Document Version: 1.0*  
*Last Updated: [Current Date]*  
*Project: AmberClick90 (AVRDU.Enc.Mat)*



## Prompt:
12/25/2025, 7:38:04 PM

One of the qustion to log to look into in the future is what are the different grays available as filament for the 3d printed case. Another question is around the keycap compatibility for matias switches (click and silent), as well as interchangeability of pcb footprint between the click and silent matias switches. We’ll want to add a link to the datasheet in this document. There will be a fuse between vbus and vdd. Make a list of all the outstanding questions that i had noted in the document i provided. We could have a software reset key on the 3rd layer as well. Frok the datasheet, I believe the GPR values also survive a reset button (pin) and a direct (non watchdog) software reset as well, so we can research that. We’ll have to research what are the different options for the reserved bootloader section in flash and likely be ‘greedy’ (maximize space available for the keyboard firmware). Look into whether we should present to the host the bootloader hid using a different hid device name (eg AmberClickFWUpdMode). Need to look into smd vs th diodes for the switches. MCU as close to usb connector as possible, but don’t compromize the location of leds and the planned location of switches and encoder. I think we’re missing VDD and GND for the net names. There is no analog ground. we will use tca for pwm. we’ll also pull the matias switch datasheet for completeness. firmware development can start with an avr64du32 nano dev board.



## Response:
12/25/2025, 7:41:10 PM

# AmberClick90 (AVRDU.Enc.Mat) - Technical Specification

## 1. Project Overview

**AmberClick90** is a custom 90-key USB keyboard featuring:

- **Physical Layout**: 90 keys arranged in a 10×9 matrix (physically 5 rows × 18 columns)
- **Switches**: Matias Click mechanical switches
- **Encoder**: One ALPS EC11 rotary encoder with integrated push switch
- **Connectivity**: USB Full-Speed via USB Mini-B connector
- **Indicators**: 3 amber LEDs (1 power, 2 PWM-controlled indicators)
- **Aesthetics**: Minimalist black PCB with minimal grey 3D-printed case
- **Programming**: UPDI for initial/recovery flashing, USB HID bootloader for updates

---

## 2. Hardware Specifications

### 2.1 Microcontroller: AVR64DU32

**Key Specifications:**
- 64 KB Flash memory
- 8 KB SRAM
- 24 MHz internal oscillator
- 32-pin package with 23 usable GPIO pins
- Native USB Full-Speed (no external PHY required)
- UPDI programming interface
- Internal USB regulator (3.3V)
- Operates at 5V input (from USB)

**Datasheet:** [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)

**Advantages over ATmega32U4:**
- 2× flash capacity
- 3× SRAM capacity
- Better USB implementation
- No external crystal required
- UPDI instead of ISP programming
- More GPIO pins (23 vs ~23 on 32U4, but cleaner power design)

### 2.2 Power Configuration

**Design**: USB Bus Powered (Datasheet Power Configuration 5b)

- VBUS → Polyfuse → VDD (5V domain from USB host)
- Internal regulator provides VUSB (3.3V for USB core)

**Decoupling Requirements:**
- Bulk capacitor on VDD near USB connector (≤10µF per USB specification)
- Local decoupling capacitors near all MCU VDD pins
- VUSB pin decoupled to ground only (no external load)

**Power Protection:**
- Polyfuse between VBUS and VDD (500mA rating)

### 2.3 GPIO Pin Allocation

**Total Usable GPIO: 23 pins**

| Function | Pin Assignment | Count |
|----------|----------------|-------|
| **Matrix Columns (9)** | PA0–PA3, PD1–PD5 | 9 |
| **Matrix Rows (10)** | PC3, PD0, PD6, PD7, PF0–PF5 | 10 |
| **Encoder A/B** | PA6, PA7 | 2 |
| **PWM Indicator LEDs** | PA4 (LED A), PA5 (LED B) | 2 |
| **Total** | | **23** |

**Pin Grouping Rationale:**
- Ports grouped to minimize register access during scanning
- Faster matrix scanning performance
- Cleaner PCB routing
- Reduced firmware complexity

**Reserved Pins (not counted as GPIO):**
- RESET pin (connected to encoder push switch)
- UPDI pin (programming interface)

### 2.4 Key Matrix

**Configuration**: 10 rows × 9 columns = 90 keys

**Implementation:**
- Matias Click switches (or Matias Quiet Click - see Section 14 Outstanding Questions)
- One diode per switch (SMD or TH - see Section 14 Outstanding Questions)
- Standard keyboard matrix configuration

**Switch Datasheet:** [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)

**Scanning Method:**
- Column-driven or row-driven (TBD during firmware development)
- Debouncing in firmware (~1-2ms typical)
- Scan rate: ~1-2 kHz typical

### 2.5 Rotary Encoder

**Model**: ALPS EC11 with detent clicks

**Datasheet:** [ALPS EC11 Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)

**Connections:**
- Encoder Phase A → PA6
- Encoder Phase B → PA7
- Integrated push switch → RESET pin (hardware reset only)

**Firmware Handling:**
- Quadrature decoding in main scan loop
- State machine or table-driven decoder
- Light debouncing (1-2ms) due to mechanical bounce
- Can implement "smart" Alt-Tab behavior (hold Alt across multiple Tab presses)

**Encoder Actions (Example):**
- Clockwise: Volume up / Alt+Tab
- Counter-clockwise: Volume down / Alt+Shift+Tab
- Can be mapped to macros stored in flash

### 2.6 LED Configuration

#### Power LED (Non-PWM)
- **Type**: 3mm through-hole amber LED, diffused
- **Connection**: VDD (5V) with current-limiting resistor
- **Current**: 0.2-0.8mA (typically 0.3-0.5mA)
- **Brightness**: Fixed, intentionally dim
- **Design Method**: Test resistor value in very dim environment to ensure non-annoying brightness while remaining visible in bright indoor lighting

#### Indicator LEDs (2× PWM-Controlled)
- **Type**: 3mm through-hole amber LEDs, diffused
- **Connections**: 
  - LED A → PA4 (TCA PWM capable)
  - LED B → PA5 (TCA PWM capable)
- **Target Current**: ~5mA per LED at 100% duty cycle
- **Forward Voltage**: ~2.0V (typical amber)
- **Resistor Calculation**: R ≈ (3.3V - 2.0V) / 0.005A ≈ 270Ω

**PWM Configuration:**
- Timer: TCA (Timer Counter A)
- Frequency: ≥500 Hz, preferably 1-2 kHz
- Resolution: 8-bit (0-255)
- Default brightness stored in Flash
- Runtime brightness stored in SRAM
- Brightness range: 5-10% minimum, 100% maximum

**Brightness Control:**
```c
#define LED_BRIGHTNESS_DEFAULT 128  // Flash (compile-time)
uint8_t led_brightness;              // SRAM (runtime)
```

**LED Advantages (3mm Diffused):**
- Smaller emitting area reduces glare
- Diffusion hides PWM artifacts
- Softer appearance in low light
- 60-100° viewing angle ideal for indicators

### 2.7 USB Interface

**Connector**: USB Mini-B, through-hole

**Design Requirements:**
- MCU placed as close to USB connector as possible (without compromising LED, switch, and encoder placement)
- Route D+/D- as short, matched-length differential pair
- Minimize stubs and avoid vias if possible
- Series resistors on D+ and D- lines (typically 22-33Ω)
- Internal USB pull-up (if MCU provides) or external 1.5kΩ on D+
- USB bulk capacitor (≤10µF total per USB spec)
- Optional: Low-capacitance USB ESD protection diode array
- Polyfuse (500mA) between VBUS and VDD

**USB Operation:**
- No external oscillator required
- Internal 24MHz oscillator with USB clock recovery (SOF-based)
- USB Full-Speed compliant

### 2.8 Programming Interfaces

#### UPDI Header
- **Location**: PCB edge (top edge preferred for case design, or alternate edge for cleaner routing)
- **Pins**: 3 or 4 pins (consult AVR64DU32 datasheet section for exact pinout)
- **Purpose**: 
  - Initial firmware programming
  - Recovery/unbrick capability
  - Development/debugging
- **Critical**: Never disable UPDI via fuses

#### USB Bootloader (HID)
- **Type**: HID bootloader (keyboard-class device)
- **Entry Method**: Firmware key combination (Fn + LeftCtrl + RightCtrl)
- **Device Name**: Research whether to use different HID device name (e.g., "AmberClick90 FW Update Mode" vs "AmberClick90")
- **Advantages**:
  - No drivers required on any OS
  - Same device class as keyboard
  - Works on locked-down systems
  - QMK-compatible workflow

---

## 3. Firmware Architecture

### 3.1 Development Toolchain

**Core Tools:**
- **Compiler**: avr-gcc
- **Editor**: Geany
- **UPDI Programmer**: SerialUPDI or similar (research required)
- **USB Flashing**: Custom HID tool (research QMK-style tools)
- **Development Board**: AVR64DU32 Nano dev board (for initial firmware development)

**Architecture**: Bare-metal + TinyUSB + minimal HAL

### 3.2 Firmware Structure

**Typical HAL Organization:**
```
/src
  main.c
  usb_descriptors.c
  keyboard.c
  matrix.c
  encoder.c
  hal_gpio.c
  hal_timer.c
  config.h
  bootloader.c
```

### 3.3 Memory Layout

#### Flash (64 KB total)
```
|------------------------| 0x0000
| HID Bootloader         |
| (4-6 KB, ~6-9%)        |
| Research: maximize     |
| app firmware space     |
|------------------------|
| Application Firmware   |
| (18-22 KB estimated)   |
| - Keyboard logic       |
| - Matrix scanning      |
| - Layer engine         |
| - Macro engine         |
| - TinyUSB stack        |
|------------------------|
| Unused                 |
| (36-42 KB free)        |
|------------------------| 0xFFFF
```

**Note**: Research bootloader section configuration options to maximize space available for keyboard firmware.

#### Flash Usage Breakdown (Application Only)

| Component | Size |
|-----------|------|
| Startup + HAL (GPIO, timer) | 1.5-2 KB |
| Matrix scanning (90 keys) | 1-1.5 KB |
| Debouncing logic | 0.5-1 KB |
| Layer engine (2 layers) | 0.5 KB |
| Macro engine (simple) | 0.5-1 KB |
| Keymap data (2×90 keys) | ~360 bytes |
| Keyboard logic glue | ~1 KB |
| **Non-USB Subtotal** | **6-8 KB** |
| TinyUSB core | 8-10 KB |
| HID class (keyboard, NKRO) | 3-4 KB |
| USB descriptors | ~0.5 KB |
| **USB Subtotal** | **12-14 KB** |
| **Total Application** | **18-22 KB** |

#### SRAM (8 KB total)

| Component | Size |
|-----------|------|
| Key matrix state (90 keys) | ~90 bytes |
| Debounce buffers | ~180 bytes |
| Macro buffers | 64-128 bytes |
| USB HID reports | 64-128 bytes |
| TinyUSB buffers | 1-1.5 KB |
| Stack | ~1 KB |
| **Total Used** | **2.5-3.5 KB** |
| **Free** | **4-5 KB** |

### 3.4 Data Storage Strategy

**Flash (Read-Only at Runtime):**
- Default keymaps (2 layers × 90 keys)
- Macro definitions
- USB descriptors
- Default LED brightness values
- Encoder action mappings

**SRAM (Runtime State):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness
- Macro execution state
- Current encoder position

**Rationale:**
- Keymaps are constants defined at compile-time
- Flash reads are single-cycle on AVR DU
- No performance penalty for flash-based lookup tables
- Matrix scanning is much slower than flash access

### 3.5 Firmware Features

#### Layer System
- 2 main layers with real keycodes
- Optional 3rd layer for system functions:
  - LED brightness adjustment
  - Bootloader entry
  - Software reset key
  - System configuration

#### Macro Support
- Simple macros (key sequences)
- Example: One key → Ctrl+C
- Stored in flash
- Execution state in SRAM

#### NKRO (N-Key Rollover)
- Full NKRO HID descriptor (research required)
- All 90 keys can be pressed simultaneously
- USB HID report format to be defined

#### Encoder Features
- Quadrature decoding
- Macro dispatch on rotation
- Optional "smart" Alt-Tab (hold Alt across rotations)
- Configurable actions per layer

#### Debouncing
- Matrix: Standard keyboard debouncing (~5-20ms typical)
- Encoder: Light debouncing (1-2ms) due to increased mechanical bounce

### 3.6 USB Device Configuration

**USB Descriptors (TBD):**
- Device name: "AmberClick90"
- Manufacturer: [Your name/handle]
- Bootloader string: Research whether to use different name (e.g., "AmberClick90 FW Update Mode")
- Version tag: "AC90-r1"

**HID Report Descriptor:**
- NKRO keyboard format (research required)
- Separate descriptors for bootloader vs application

---

## 4. Bootloader Implementation

### 4.1 Bootloader Type: HID (Keyboard-Class)

**Specifications:**
- Flash size: 4-6 KB (~6-9% of total) - Research optimal bootloader section size
- SRAM: 300-600 bytes
- Enumeration: USB HID device
- Protocol: Custom (similar to QMK)
- No drivers required on host

**Advantages:**
- ✓ Driver-less operation on all OS
- ✓ Same device class as keyboard
- ✓ Works on restricted systems
- ✓ Easy integration with existing workflows

### 4.2 Bootloader Entry Mechanism

**Primary Method**: Firmware Key Combination
- **Combo**: Fn + LeftCtrl + RightCtrl
- **Detection**: During normal matrix scanning
- **Process**: Firmware-initiated reset into bootloader

**Additional Method**: Software Reset Key
- **Location**: 3rd layer (system layer)
- **Process**: Direct software reset into bootloader

**Secondary Method**: Manual Recovery
- **Method**: UPDI programming
- **Purpose**: Unbrick/development only

### 4.3 Bootloader Request Flow

#### Step 1: Key Combination Detection
```c
if (fn_layer_active &&
    key_is_pressed(KEY_LEFT_CTRL) &&
    key_is_pressed(KEY_RIGHT_CTRL)) {
    request_bootloader();
}

// Or software reset key on layer 3
if (layer3_active && key_is_pressed(KEY_SW_RESET)) {
    request_bootloader();
}
```

#### Step 2: Set Boot Flag (GPR Method)

**Magic Value Storage:**
- Use General Purpose Register (r2 or r3 recommended)
- Write magic value: `0x42`
- **Research Note**: Datasheet suggests GPR values may survive RESET pin and direct software reset in addition to watchdog reset - verify this behavior

```c
#define BOOT_MAGIC 0x42

void request_bootloader(void) {
    asm volatile (
        "ldi r2, %0\n"
        :
        : "M" (BOOT_MAGIC)
    );
    wdt_enable(WDTO_15MS);
    while (1) {}  // Wait for watchdog reset
}
```

#### Step 3: Watchdog Reset
- Use watchdog timer (NOT software jump)
- Cleanest reset method
- Preserves GPR values
- Research: May also preserve GPR on RESET pin and direct software reset

#### Step 4: Early Boot Check

**Must occur before:**
- .data initialization
- .bss clearing
- USB initialization
- Stack setup (ideally)

```c
// In bootloader startup (before C runtime init)
if (r2 == BOOT_MAGIC) {
    r2 = 0;  // Clear immediately
    stay_in_bootloader();
} else {
    jump_to_application();
}
```

### 4.4 Reset Behavior Summary

| Reset Type | GPR Preserved? | Use Case |
|------------|----------------|----------|
| Power-on | ❌ No | Normal startup |
| Brown-out | ❌ No | Voltage drop |
| Watchdog | ✅ Yes | Bootloader entry |
| External (RESET pin) | ❓ Research | Manual reset / Encoder button |
| Software Reset (direct) | ❓ Research | Layer 3 reset key |

**Research Required**: Verify GPR preservation behavior for RESET pin and direct software reset methods.

### 4.5 Bootloader State Machine

**Boot Flow:**
```
Power On / Reset
    ↓
Bootloader Starts
    ↓
Check GPR for BOOT_MAGIC
    ↓
   [Magic Present?]
    ↓              ↓
   Yes            No
    ↓              ↓
Stay in         Jump to
Bootloader      Application
(USB HID)       (Keyboard)
```

**User-Visible Behavior:**

| Scenario | Result |
|----------|--------|
| Normal power-on | Keyboard starts immediately |
| Fn+Ctrl+Ctrl pressed | Keyboard → Bootloader mode |
| Layer 3 reset key | Keyboard → Bootloader mode |
| Encoder button (RESET) | Firmware restart (research GPR behavior) |
| UPDI programming | Direct flash write |

### 4.6 Bootloader-Application Communication

**Fuse Configuration:**
- Reserve bootloader section in flash (research optimal size - maximize app space)
- Protect bootloader from application overwrites
- Never disable UPDI

**Jump Mechanism:**
```c
#define BOOTLOADER_ADDR 0x0000  // Adjust per fuse settings

void jump_to_bootloader(void) {
    cli();  // Disable interrupts
    ((void (*)(void))BOOTLOADER_ADDR)();
}
```

### 4.7 USB Enumeration Behavior

**From Host Perspective:**

1. **Normal Operation**: Device enumerates as "AmberClick90" keyboard
2. **Bootloader Entry**: 
   - Keyboard disconnects
   - New HID device appears (bootloader)
   - Research: Consider using different device name (e.g., "AmberClick90 FW Update Mode")
3. **Firmware Update**: Host tool sends firmware via HID reports
4. **Completion**: Device resets, keyboard re-enumerates

---

## 5. Component Selection

### 5.1 Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **MCU** | AVR64DU32 | 1 | Use internal oscillator |
| **USB Connector** | USB Mini-B, through-hole | 1 | Full-Speed capable |
| **UPDI Header** | 3-4 pin header | 1 | See datasheet for pinout |
| **USB Series Resistors** | 22-33Ω, 0805 | 2 | For D+, D- |
| **USB Bulk Cap** | 10µF max, near connector | 1 | Per USB specification |
| **VDD Decoupling Caps** | 100nF, 0805 | Per VDD pin | Near each VDD pin |
| **VUSB Decoupling Cap** | 100nF, 0805 | 1 | Near VUSB pin |
| **Polyfuse** | 500mA, resettable | 1 | Between VBUS and VDD |
| **ESD Protection** | Low-cap USB diode array | 1 | Optional |
| **Switches** | Matias Click (or Quiet) | 90 | Mechanical switches |
| **Diodes** | 1N4148 or equivalent | 90 | SMD or TH - see Section 14 |
| **Rotary Encoder** | ALPS EC11 with switch | 1 | With detent clicks |
| **Power LED** | 3mm amber, diffused, TH | 1 | Fixed brightness |
| **Indicator LEDs** | 3mm amber, diffused, TH | 2 | PWM-controlled |
| **LED Resistors** | TBD (see calculations) | 3 | Near LEDs |

### 5.2 LED Resistor Calculations

#### Power LED
- **Target**: Dim but visible (0.3-0.5mA typical)
- **Formula**: R = (VDD - Vf) / I = (5V - 2.0V) / 0.0005A ≈ 6kΩ
- **Selection Method**: Test in very dim environment to ensure non-annoying brightness

#### Indicator LEDs (PWM)
- **Target**: 5mA at 100% duty cycle (bright indoor visibility)
- **Formula**: R = (3.3V - 2.0V) / 0.005A = 260Ω
- **Standard Value**: 270Ω
- **Power**: P = I² × R = (0.005)² × 270 ≈ 7mW
- **Resistor Placement**: Near LED, not MCU (reduces EMI, easier rework)

---

## 6. PCB Design Guidelines

### 6.1 Physical Layout

**Component Placement:**
- USB connector: Top edge, aligned with Fn key
- MCU: As close to USB connector as possible (without compromising LED, switch, and encoder placement)
- Rotary encoder: Top-left corner
- Three LEDs: Horizontal arrangement near top edge
- UPDI header: Top edge (easier case design) or alternate edge for cleaner routing
- Switches: Standard keyboard grid layout

**Silkscreen:**
- Project name: "AmberClick90"
- Trim designation: "(AVRDU.Enc.Mat)"
- Typographic logo design (TBD)
- Minimalist, monochrome aesthetic
- Technical/instrument-like appearance

### 6.2 Design Specifications

- **PCB**: 2-layer, black solder mask
- **Case**: Minimal, grey 3D-printed (research filament grey color options - see Section 14)
- **Aesthetic**: Minimalist, technical

### 6.3 Net Naming Convention

**Power:**
- VDD (5V)
- VUSB (3.3V internal)
- GND

**Rows:**
- ROW_0 through ROW_9

**Columns:**
- COL_0 through COL_8

**Encoder:**
- ENC_A, ENC_B
- RESET (encoder switch)

**LEDs:**
- LED_PWR (power LED)
- LED_IND_A, LED_IND_B (indicators)

**USB:**
- USB_DP, USB_DM
- VBUS

**Programming:**
- UPDI
- RESET

### 6.4 Critical Routing

**USB Differential Pair:**
- Keep D+ and D- as short, matched-length traces
- Minimize stubs
- Avoid vias if possible
- Route as differential pair (typical impedance: 90Ω)

**Power:**
- Wide traces for VDD/GND
- Single ground plane (no separate analog ground)
- Star-ground from USB connector
- Polyfuse between VBUS and VDD

**Matrix:**
- Keep row/column traces away from USB lines
- Group by port for easier scanning

---

## 7. Firmware Development Roadmap

### 7.1 Initial Development Tasks

1. **Research Phase**
   - Study QMK ATmega32U4 implementation
   - Review DXCore for AVR DU specifics:
     - Pin mapping
     - Clock configuration
     - Fuse settings
     - Startup code
     - Linker scripts
     - UPDI flashing methods
   - Research TinyUSB integration
   - Define NKRO HID descriptor

2. **HAL Development**
   - GPIO abstraction (port-based access)
   - TCA timer setup for PWM
   - Watchdog configuration
   - Clock initialization

3. **Core Firmware**
   - Matrix scanning loop
   - Debouncing algorithm
   - Layer switching logic (2 main + 1 system layer)
   - Macro engine

4. **USB Stack**
   - TinyUSB integration
   - HID keyboard descriptor (NKRO)
   - USB enumeration
   - Device strings

5. **Bootloader**
   - HID bootloader implementation
   - GPR boot flag mechanism
   - Flash write routines
   - Application jump logic

6. **Development Environment**
   - Start firmware development using AVR64DU32 Nano dev board
   - Test basic functionality before PCB fabrication

### 7.2 Questions for LLM Assistant

**HAL & Architecture:**
- Sketch minimal HAL API for AVR64DU32
- Show TinyUSB HID report descriptor for NKRO
- Design HID bootloader strategy
- Provide bootloader entry mechanism (key combo vs reset)
- Show TinyUSB-compatible HID descriptor split (bootloader vs app)

**Matrix Scanning:**
- Show minimal scan loop for exact pin map (10 rows, 9 columns)
- Compare row-driven vs column-driven scanning
- Help decide pull-ups vs pull-downs
- Walk through diode orientation trade-offs

**Encoder Integration:**
- Sketch robust quadrature decoder
- Design Alt-Tab "smart encoder" behavior
- Suggest GPIO + pull-up configuration
- Show integration into matrix scan loop
- Show firmware logic for decoding encoders

**LED Control:**
- Suggest exact TCA PWM timer setup for PA4, PA5
- Choose PWM frequency that won't alias with matrix scanning
- Show exact TCA PWM timer config for AVR64DU32
- Recommend brightness step sizes
- Add cheap gamma correction
- Ensure PWM doesn't interfere with USB timing

**Power Analysis:**
- Check worst-case power consumption with everything on
- Estimate average current draw from USB
- Estimate maximum current draw (all LEDs + matrix)

**Bootloader Details:**
- Draft clean bootloader.c module
- Show minimal HID bootloader state machine
- Explain correct RESET button wiring (per datasheet)

**Clock & Fuses:**
- Research correct fuse settings (never disable UPDI)
- Select appropriate clock speed
- Evaluate sleep + wake-on-interrupt approach for power saving

---

## 8. Firmware Configuration Details

### 8.1 Clock Configuration

- **Primary Clock**: 24 MHz internal oscillator
- **USB Clock**: Derived from internal oscillator with USB clock recovery
- **Sleep Mode**: Evaluate wake-on-interrupt for power efficiency (research required)

### 8.2 Fuse Settings (Critical)

**Must Research:**
- Bootloader section size and location (maximize application firmware space)
- UPDI enable (NEVER disable)
- Brown-out detection level
- Watchdog configuration
- Reset pin configuration (tied to encoder switch)

### 8.3 Matrix Scanning Parameters

**Configuration:**
- Scan rate: 1-2 kHz typical
- Debounce time: 5-20ms (tune during testing)
- Pull configuration: TBD (pull-up vs pull-down)
- Diode orientation: TBD (COL2ROW vs ROW2COL)

**Port Optimization:**
- Rows grouped on single port where possible → single register read
- Columns distributed for routing convenience
- Minimize port switching during scan

### 8.4 PWM Configuration

**Timer Selection:**
- Use TCA (Timer Counter A) for PWM
- Single timer can drive both PWM channels (PA4, PA5)

**PWM Parameters:**
- Frequency: 1-2 kHz (avoid aliasing with matrix scan)
- Resolution: 8-bit (0-255)
- Brightness steps: 16-32 levels recommended
- Gamma correction: Optional, low-cost implementation

**Brightness Control:**
```c
// Default stored in Flash
#define LED_BRIGHTNESS_DEFAULT 128

// Runtime variable in SRAM
uint8_t led_brightness_a;
uint8_t led_brightness_b;

// Initialize at startup
led_brightness_a = LED_BRIGHTNESS_DEFAULT;
led_brightness_b = LED_BRIGHTNESS_DEFAULT;
```

### 8.5 Encoder Configuration

**Decoder Type:**
- State machine (preferred) or table-driven
- Handles mechanical bounce
- Integrates into main scan loop

**Sampling:**
- Poll during matrix scan (1-2 kHz)
- Or use pin-change interrupt (optional)

**Action Mapping (Example):**
```c
// Stored in Flash
const encoder_action_t encoder_map[] = {
    [LAYER_0] = { .cw = MACRO_VOL_UP,    .ccw = MACRO_VOL_DOWN },
    [LAYER_1] = { .cw = MACRO_ALT_TAB,   .ccw = MACRO_ALT_SHIFT_TAB }
};
```

---

## 9. Design Philosophy & Best Practices

### 9.1 Memory Management

**Flash Usage:**
- Store all constant data (keymaps, macros, descriptors)
- No runtime writes (except via bootloader)
- Maximize utilization of 64KB capacity
- Research bootloader section options to maximize application space

**SRAM Usage:**
- Minimize static allocation
- Use for runtime state only
- Monitor stack usage carefully

### 9.2 Code Quality

**Style:**
- Direct register access
- Minimal abstraction where performance critical
- Clean HAL for portability
- Well-commented for maintainability

**Portability:**
- Borrow concepts from QMK and DXCore
- Adapt for AVR DU specifics
- Document deviations from reference implementations

### 9.3 User Experience

**Boot Time:**
- Instant firmware start (<100ms)
- Quick bootloader check (no delay)

**Responsiveness:**
- Low-latency matrix scanning
- Immediate key feedback
- Smooth encoder operation

**Reliability:**
- Robust debouncing
- No false triggering
- Stable USB enumeration

---

## 10. Testing & Validation

### 10.1 Hardware Testing

- [ ] USB enumeration on multiple hosts (Windows, macOS, Linux)
- [ ] All 90 keys register correctly
- [ ] Encoder rotation and detents
- [ ] Reset button (encoder push)
- [ ] All three LEDs illuminate
- [ ] PWM dimming works correctly
- [ ] UPDI programming successful
- [ ] Bootloader entry via key combo
- [ ] Software reset key (layer 3)

### 10.2 Firmware Testing

- [ ] Matrix scanning accuracy (no ghost keys)
- [ ] Debouncing effectiveness
- [ ] Layer switching (2 main + 1 system layer)
- [ ] Macro execution
- [ ] NKRO verification (6+ simultaneous keys)
- [ ] Encoder decoding accuracy
- [ ] LED brightness control
- [ ] Bootloader mode entry/exit (multiple methods)
- [ ] Firmware update via USB

### 10.3 Power Testing

- [ ] USB current draw (idle)
- [ ] USB current draw (all keys pressed)
- [ ] USB current draw (all LEDs at 100%)
- [ ] Verify ≤500mA total (USB spec compliance)
- [ ] Polyfuse operation verification

---

## 11. Future Enhancements (Optional)

**Pin-Saving Techniques (if needed):**
- Share LEDs with matrix columns (LED active only when column driven)
- Encoder pins as matrix keys (careful decoding required)
- Note: Current design has sufficient GPIO, these are optional

**Advanced Features:**
- VIA/VIAL support for runtime keymap editing
- RGB underglow (if desired)
- Audio output (piezo buzzer)
- Additional encoders
- OLED display

---

## 12. References & Resources

**Datasheets:**
- [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)
- [ALPS EC11 Encoder Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244



## Prompt:
12/25/2025, 8:05:50 PM

Continue



## Response:
12/25/2025, 8:06:22 PM

G1/)
- [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)
- USB 2.0 specification

**Software:**
- QMK Firmware (reference implementation)
- DXCore (AVR DU support)
- TinyUSB (USB stack)
- avr-gcc documentation

**Standards:**
- USB HID Usage Tables
- USB 2.0 Full-Speed specification
- Keyboard scan code standards

---

## 13. Typographic Logo Design

**Requirements:**
- Simple, silkscreen-friendly design
- Minimalist aesthetic
- Technical/instrument-like appearance
- Monochrome (black PCB, white silkscreen)
- Incorporates "AmberClick90" or "AC90" branding

**Design Status**: TBD

---

## 14. Outstanding Research Questions

### 14.1 Hardware Questions

1. **3D Printed Case**
   - What grey filament colors are available?
   - Which grey shade best matches minimalist aesthetic?
   - PLA, PETG, or ABS for case material?

2. **Matias Switches**
   - Keycap compatibility for Matias Click switches
   - Keycap compatibility for Matias Quiet Click switches
   - PCB footprint interchangeability between Click and Quiet Click variants
   - Source for Matias-compatible keycaps

3. **Matrix Diodes**
   - SMD vs through-hole diodes for switch matrix?
   - If SMD: SOD-123 or SOD-323 package?
   - If TH: standard DO-35 package?
   - PCB routing considerations for each option

4. **UPDI Programming**
   - UPDI programmer dongle options (SerialUPDI vs hardware programmer)
   - UPDI software tools (avrdude support for AVR DU?)
   - UPDI header pinout (3-pin vs 4-pin configuration)

5. **USB Bootloader Tools**
   - QMK Toolbox compatibility?
   - Custom flashing tool requirements?
   - Host-side software for HID bootloader

### 14.2 Firmware Questions

6. **Reset Behavior**
   - Do GPR values survive RESET pin assertion? (datasheet suggests yes - verify)
   - Do GPR values survive direct software reset? (datasheet suggests yes - verify)
   - Testing required to confirm GPR preservation across reset types

7. **Bootloader Configuration**
   - What are the bootloader section size options in fuses?
   - How to maximize application firmware space?
   - Optimal bootloader section configuration for 64KB flash

8. **USB Device Naming**
   - Should bootloader present different HID device name?
   - "AmberClick90" vs "AmberClick90 FW Update Mode"
   - User experience implications of name change

9. **Clock & Power**
   - Optimal clock speed settings and fuse configuration
   - Sleep mode implementation for power savings
   - Wake-on-interrupt approach feasibility

10. **Matrix Scanning**
    - Row-driven vs column-driven scanning performance comparison
    - Pull-up vs pull-down resistor configuration
    - Diode orientation (COL2ROW vs ROW2COL) trade-offs

11. **NKRO Implementation**
    - HID descriptor format for full NKRO
    - USB report structure for 90-key NKRO
    - Compatibility considerations across operating systems

12. **USB HID Descriptor**
    - Setup requirements for custom HID descriptors
    - Split descriptor approach (bootloader vs application)
    - TinyUSB integration specifics

13. **DXCore Research**
    - Pin mapping conventions for AVR DU
    - Clock and fuse settings used by DXCore
    - Startup code and linker script templates
    - UPDI flashing methodology in DXCore

14. **PWM Timing**
    - TCA timer configuration for dual-channel PWM
    - PWM frequency selection to avoid matrix scan aliasing
    - Gamma correction implementation (lookup table vs calculation)

15. **Encoder Implementation**
    - Optimal quadrature decoder algorithm
    - "Smart" Alt-Tab behavior implementation details
    - Encoder state initialization to avoid phantom rotations on boot

### 14.3 Power & Safety Questions

16. **Current Draw Analysis**
    - Worst-case current consumption calculation
    - Average current draw during typical use
    - Peak current during all-keys-pressed + all-LEDs-on scenario
    - Polyfuse rating verification (500mA appropriate?)

### 14.4 PCB & Assembly Questions

17. **Component Sourcing**
    - AVR64DU32 availability and pricing
    - Matias switch availability (Click and Quiet Click)
    - Matias-compatible keycap sets
    - Through-hole vs SMD component cost comparison

18. **MCU Placement**
    - Optimal MCU location balancing:
      - Proximity to USB connector
      - LED placement constraints
      - Switch matrix routing
      - Encoder location
      - Overall PCB layout efficiency

---

## 15. Document Status

This specification represents the **current consolidated design** as of this revision and serves as:

- Complete reference for schematic and PCB design
- Firmware architecture guide
- Component selection reference
- Testing checklist
- Handoff document for future development
- Comprehensive list of outstanding research questions

**Next Steps:**
1. Research outstanding questions (Section 14)
2. Finalize typographic logo design
3. Begin firmware development using AVR64DU32 Nano dev board
4. Create KiCad schematic
5. Complete PCB layout
6. Implement and test bootloader

---

*Document Version: 2.0*  
*Last Updated: December 25, 2025*  
*Project: AmberClick90 (AVRDU.Enc.Mat)*



## Prompt:
12/25/2025, 8:09:15 PM

it looks like the matias switches use alps keycaps. they can be found here (for the clicky one): https://matias.ca/switches/click/. The software reset key on layer 3 is just for reset (fw restart), not for entry into the FW update mode, so please correct sections 4.2 and 4.3 and 4.4 and 4.5. As the AVR64DU32 is the best mcu in the AVR-DU family currently, we could develop the FW with that mcu in mind only, with some very moderate flexibility to be forward compatible with AVR-DU variants that may have more than 64K of flash, 8kb of ram, or 32 pins.



## Response:
12/25/2025, 8:12:18 PM

# AmberClick90 (AVRDU.Enc.Mat) - Technical Specification

## 1. Project Overview

**AmberClick90** is a custom 90-key USB keyboard featuring:

- **Physical Layout**: 90 keys arranged in a 10×9 matrix (physically 5 rows × 18 columns)
- **Switches**: Matias Click mechanical switches (Alps-compatible)
- **Encoder**: One ALPS EC11 rotary encoder with integrated push switch
- **Connectivity**: USB Full-Speed via USB Mini-B connector
- **Indicators**: 3 amber LEDs (1 power, 2 PWM-controlled indicators)
- **Aesthetics**: Minimalist black PCB with minimal grey 3D-printed case
- **Programming**: UPDI for initial/recovery flashing, USB HID bootloader for updates

---

## 2. Hardware Specifications

### 2.1 Microcontroller: AVR64DU32

**Key Specifications:**
- 64 KB Flash memory
- 8 KB SRAM
- 24 MHz internal oscillator
- 32-pin package with 23 usable GPIO pins
- Native USB Full-Speed (no external PHY required)
- UPDI programming interface
- Internal USB regulator (3.3V)
- Operates at 5V input (from USB)

**Datasheet:** [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)

**Design Philosophy:**
- Firmware developed specifically for AVR64DU32 (best MCU in AVR-DU family currently)
- Moderate flexibility for forward compatibility with future AVR-DU variants featuring:
  - More than 64 KB Flash
  - More than 8 KB SRAM
  - More than 32 pins
- No backward compatibility concerns with smaller AVR-DU parts

**Advantages over ATmega32U4:**
- 2× flash capacity
- 3× SRAM capacity
- Better USB implementation
- No external crystal required
- UPDI instead of ISP programming
- More GPIO pins (23 vs ~23 on 32U4, but cleaner power design)

### 2.2 Power Configuration

**Design**: USB Bus Powered (Datasheet Power Configuration 5b)

- VBUS → Polyfuse → VDD (5V domain from USB host)
- Internal regulator provides VUSB (3.3V for USB core)

**Decoupling Requirements:**
- Bulk capacitor on VDD near USB connector (≤10µF per USB specification)
- Local decoupling capacitors near all MCU VDD pins
- VUSB pin decoupled to ground only (no external load)

**Power Protection:**
- Polyfuse between VBUS and VDD (500mA rating)

### 2.3 GPIO Pin Allocation

**Total Usable GPIO: 23 pins**

| Function | Pin Assignment | Count |
|----------|----------------|-------|
| **Matrix Columns (9)** | PA0–PA3, PD1–PD5 | 9 |
| **Matrix Rows (10)** | PC3, PD0, PD6, PD7, PF0–PF5 | 10 |
| **Encoder A/B** | PA6, PA7 | 2 |
| **PWM Indicator LEDs** | PA4 (LED A), PA5 (LED B) | 2 |
| **Total** | | **23** |

**Pin Grouping Rationale:**
- Ports grouped to minimize register access during scanning
- Faster matrix scanning performance
- Cleaner PCB routing
- Reduced firmware complexity

**Reserved Pins (not counted as GPIO):**
- RESET pin (connected to encoder push switch)
- UPDI pin (programming interface)

### 2.4 Key Matrix

**Configuration**: 10 rows × 9 columns = 90 keys

**Implementation:**
- Matias Click switches (Alps-compatible)
- Alps-compatible keycaps ([source](https://matias.ca/switches/click/))
- One diode per switch (SMD or TH - see Section 14 Outstanding Questions)
- Standard keyboard matrix configuration

**Switch Datasheet:** [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)

**Keycap Compatibility:**
- Matias switches use Alps mount (not Cherry MX)
- Alps-compatible keycaps available from Matias and other suppliers
- Verify keycap set availability before finalizing design

**Scanning Method:**
- Column-driven or row-driven (TBD during firmware development)
- Debouncing in firmware (~1-2ms typical)
- Scan rate: ~1-2 kHz typical

### 2.5 Rotary Encoder

**Model**: ALPS EC11 with detent clicks

**Datasheet:** [ALPS EC11 Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)

**Connections:**
- Encoder Phase A → PA6
- Encoder Phase B → PA7
- Integrated push switch → RESET pin (hardware reset only)

**Firmware Handling:**
- Quadrature decoding in main scan loop
- State machine or table-driven decoder
- Light debouncing (1-2ms) due to mechanical bounce
- Can implement "smart" Alt-Tab behavior (hold Alt across multiple Tab presses)

**Encoder Actions (Example):**
- Clockwise: Volume up / Alt+Tab
- Counter-clockwise: Volume down / Alt+Shift+Tab
- Can be mapped to macros stored in flash

### 2.6 LED Configuration

#### Power LED (Non-PWM)
- **Type**: 3mm through-hole amber LED, diffused
- **Connection**: VDD (5V) with current-limiting resistor
- **Current**: 0.2-0.8mA (typically 0.3-0.5mA)
- **Brightness**: Fixed, intentionally dim
- **Design Method**: Test resistor value in very dim environment to ensure non-annoying brightness while remaining visible in bright indoor lighting

#### Indicator LEDs (2× PWM-Controlled)
- **Type**: 3mm through-hole amber LEDs, diffused
- **Connections**: 
  - LED A → PA4 (TCA PWM capable)
  - LED B → PA5 (TCA PWM capable)
- **Target Current**: ~5mA per LED at 100% duty cycle
- **Forward Voltage**: ~2.0V (typical amber)
- **Resistor Calculation**: R ≈ (3.3V - 2.0V) / 0.005A ≈ 270Ω

**PWM Configuration:**
- Timer: TCA (Timer Counter A)
- Frequency: ≥500 Hz, preferably 1-2 kHz
- Resolution: 8-bit (0-255)
- Default brightness stored in Flash
- Runtime brightness stored in SRAM
- Brightness range: 5-10% minimum, 100% maximum

**Brightness Control:**
```c
#define LED_BRIGHTNESS_DEFAULT 128  // Flash (compile-time)
uint8_t led_brightness;              // SRAM (runtime)
```

**LED Advantages (3mm Diffused):**
- Smaller emitting area reduces glare
- Diffusion hides PWM artifacts
- Softer appearance in low light
- 60-100° viewing angle ideal for indicators

### 2.7 USB Interface

**Connector**: USB Mini-B, through-hole

**Design Requirements:**
- MCU placed as close to USB connector as possible (without compromising LED, switch, and encoder placement)
- Route D+/D- as short, matched-length differential pair
- Minimize stubs and avoid vias if possible
- Series resistors on D+ and D- lines (typically 22-33Ω)
- Internal USB pull-up (if MCU provides) or external 1.5kΩ on D+
- USB bulk capacitor (≤10µF total per USB spec)
- Optional: Low-capacitance USB ESD protection diode array
- Polyfuse (500mA) between VBUS and VDD

**USB Operation:**
- No external oscillator required
- Internal 24MHz oscillator with USB clock recovery (SOF-based)
- USB Full-Speed compliant

### 2.8 Programming Interfaces

#### UPDI Header
- **Location**: PCB edge (top edge preferred for case design, or alternate edge for cleaner routing)
- **Pins**: 3 or 4 pins (consult AVR64DU32 datasheet section for exact pinout)
- **Purpose**: 
  - Initial firmware programming
  - Recovery/unbrick capability
  - Development/debugging
- **Critical**: Never disable UPDI via fuses

#### USB Bootloader (HID)
- **Type**: HID bootloader (keyboard-class device)
- **Entry Method**: Firmware key combination (Fn + LeftCtrl + RightCtrl)
- **Device Name**: Research whether to use different HID device name (e.g., "AmberClick90 FW Update Mode" vs "AmberClick90")
- **Advantages**:
  - No drivers required on any OS
  - Same device class as keyboard
  - Works on locked-down systems
  - QMK-compatible workflow

---

## 3. Firmware Architecture

### 3.1 Development Toolchain

**Core Tools:**
- **Compiler**: avr-gcc
- **Editor**: Geany
- **UPDI Programmer**: SerialUPDI or similar (research required)
- **USB Flashing**: Custom HID tool (research QMK-style tools)
- **Development Board**: AVR64DU32 Nano dev board (for initial firmware development)

**Architecture**: Bare-metal + TinyUSB + minimal HAL

### 3.2 Firmware Structure

**Typical HAL Organization:**
```
/src
  main.c
  usb_descriptors.c
  keyboard.c
  matrix.c
  encoder.c
  hal_gpio.c
  hal_timer.c
  config.h
  bootloader.c
```

### 3.3 Memory Layout

#### Flash (64 KB total)
```
|------------------------| 0x0000
| HID Bootloader         |
| (4-6 KB, ~6-9%)        |
| Research: maximize     |
| app firmware space     |
|------------------------|
| Application Firmware   |
| (18-22 KB estimated)   |
| - Keyboard logic       |
| - Matrix scanning      |
| - Layer engine         |
| - Macro engine         |
| - TinyUSB stack        |
|------------------------|
| Unused                 |
| (36-42 KB free)        |
|------------------------| 0xFFFF
```

**Note**: Research bootloader section configuration options to maximize space available for keyboard firmware.

**Forward Compatibility**: Design allows for scaling to larger AVR-DU variants with >64KB flash if released in the future.

#### Flash Usage Breakdown (Application Only)

| Component | Size |
|-----------|------|
| Startup + HAL (GPIO, timer) | 1.5-2 KB |
| Matrix scanning (90 keys) | 1-1.5 KB |
| Debouncing logic | 0.5-1 KB |
| Layer engine (2 layers) | 0.5 KB |
| Macro engine (simple) | 0.5-1 KB |
| Keymap data (2×90 keys) | ~360 bytes |
| Keyboard logic glue | ~1 KB |
| **Non-USB Subtotal** | **6-8 KB** |
| TinyUSB core | 8-10 KB |
| HID class (keyboard, NKRO) | 3-4 KB |
| USB descriptors | ~0.5 KB |
| **USB Subtotal** | **12-14 KB** |
| **Total Application** | **18-22 KB** |

#### SRAM (8 KB total)

| Component | Size |
|-----------|------|
| Key matrix state (90 keys) | ~90 bytes |
| Debounce buffers | ~180 bytes |
| Macro buffers | 64-128 bytes |
| USB HID reports | 64-128 bytes |
| TinyUSB buffers | 1-1.5 KB |
| Stack | ~1 KB |
| **Total Used** | **2.5-3.5 KB** |
| **Free** | **4-5 KB** |

**Forward Compatibility**: Design allows for scaling to AVR-DU variants with >8KB SRAM if released in the future.

### 3.4 Data Storage Strategy

**Flash (Read-Only at Runtime):**
- Default keymaps (2 layers × 90 keys)
- Macro definitions
- USB descriptors
- Default LED brightness values
- Encoder action mappings

**SRAM (Runtime State):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness
- Macro execution state
- Current encoder position

**Rationale:**
- Keymaps are constants defined at compile-time
- Flash reads are single-cycle on AVR DU
- No performance penalty for flash-based lookup tables
- Matrix scanning is much slower than flash access

### 3.5 Firmware Features

#### Layer System
- 2 main layers with real keycodes (Layer 0, Layer 1)
- 1 system layer (Layer 2) for system functions:
  - LED brightness adjustment
  - Bootloader entry (Fn + LeftCtrl + RightCtrl → bootloader mode)
  - Software reset key (firmware restart only, not bootloader entry)
  - System configuration

#### Macro Support
- Simple macros (key sequences)
- Example: One key → Ctrl+C
- Stored in flash
- Execution state in SRAM

#### NKRO (N-Key Rollover)
- Full NKRO HID descriptor (research required)
- All 90 keys can be pressed simultaneously
- USB HID report format to be defined

#### Encoder Features
- Quadrature decoding
- Macro dispatch on rotation
- Optional "smart" Alt-Tab (hold Alt across rotations)
- Configurable actions per layer

#### Debouncing
- Matrix: Standard keyboard debouncing (~5-20ms typical)
- Encoder: Light debouncing (1-2ms) due to increased mechanical bounce

### 3.6 USB Device Configuration

**USB Descriptors (TBD):**
- Device name: "AmberClick90"
- Manufacturer: [Your name/handle]
- Bootloader string: Research whether to use different name (e.g., "AmberClick90 FW Update Mode")
- Version tag: "AC90-r1"

**HID Report Descriptor:**
- NKRO keyboard format (research required)
- Separate descriptors for bootloader vs application

---

## 4. Bootloader Implementation

### 4.1 Bootloader Type: HID (Keyboard-Class)

**Specifications:**
- Flash size: 4-6 KB (~6-9% of total) - Research optimal bootloader section size
- SRAM: 300-600 bytes
- Enumeration: USB HID device
- Protocol: Custom (similar to QMK)
- No drivers required on host

**Advantages:**
- ✓ Driver-less operation on all OS
- ✓ Same device class as keyboard
- ✓ Works on restricted systems
- ✓ Easy integration with existing workflows

### 4.2 Bootloader Entry Mechanism

**Primary Method**: Firmware Key Combination (Bootloader Entry)
- **Combo**: Fn + LeftCtrl + RightCtrl
- **Location**: Accessible from main layers (typically Layer 0 or Layer 1 via Fn key)
- **Detection**: During normal matrix scanning
- **Process**: Firmware-initiated reset into bootloader
- **Purpose**: Enter firmware update mode (USB HID bootloader)

**Additional Method**: Software Reset Key (Firmware Restart Only)
- **Location**: Layer 2 (system layer)
- **Process**: Direct software reset (watchdog or similar)
- **Purpose**: Restart keyboard firmware without entering bootloader
- **Behavior**: Normal firmware startup (not bootloader mode)
- **Use Case**: Quick firmware restart without unplugging USB

**Recovery Method**: Manual UPDI Programming
- **Method**: UPDI programming
- **Purpose**: Unbrick/development only
- **Use Case**: Corrupted firmware or bootloader

### 4.3 Bootloader Request Flow

#### Bootloader Entry (Fn + LeftCtrl + RightCtrl)

**Step 1: Key Combination Detection**
```c
// Bootloader entry - detects key combo and enters FW update mode
if (fn_layer_active &&
    key_is_pressed(KEY_LEFT_CTRL) &&
    key_is_pressed(KEY_RIGHT_CTRL)) {
    request_bootloader();  // Enter bootloader mode
}
```

**Step 2: Set Boot Flag (GPR Method)**

**Magic Value Storage:**
- Use General Purpose Register (r2 or r3 recommended)
- Write magic value: `0x42`
- **Research Note**: Datasheet suggests GPR values may survive RESET pin and direct software reset in addition to watchdog reset - verify this behavior

```c
#define BOOT_MAGIC 0x42

void request_bootloader(void) {
    asm volatile (
        "ldi r2, %0\n"
        :
        : "M" (BOOT_MAGIC)
    );
    wdt_enable(WDTO_15MS);
    while (1) {}  // Wait for watchdog reset
}
```

**Step 3: Watchdog Reset**
- Use watchdog timer (NOT software jump)
- Cleanest reset method
- Preserves GPR values
- Research: May also preserve GPR on RESET pin and direct software reset

**Step 4: Early Boot Check**

**Must occur before:**
- .data initialization
- .bss clearing
- USB initialization
- Stack setup (ideally)

```c
// In bootloader startup (before C runtime init)
if (r2 == BOOT_MAGIC) {
    r2 = 0;  // Clear immediately
    stay_in_bootloader();
} else {
    jump_to_application();
}
```

#### Software Reset (Layer 2 Reset Key)

**Step 1: Reset Key Detection**
```c
// Software reset - restarts firmware only (does NOT enter bootloader)
if (layer2_active && key_is_pressed(KEY_SW_RESET)) {
    software_reset();  // Restart firmware
}
```

**Step 2: Direct Reset (No Boot Flag)**
```c
void software_reset(void) {
    // Do NOT set boot flag - we want normal firmware startup
    wdt_enable(WDTO_15MS);
    while (1) {}  // Wait for watchdog reset
}
```

**Step 3: Normal Startup**
- Bootloader checks GPR (no magic value present)
- Bootloader immediately jumps to application
- Keyboard firmware starts normally

### 4.4 Reset Behavior Summary

| Reset Type | GPR Preserved? | Boot Flag | Result |
|------------|----------------|-----------|--------|
| Power-on | ❌ No | N/A | Normal firmware start |
| Brown-out | ❌ No | N/A | Normal firmware start |
| Watchdog (with BOOT_MAGIC) | ✅ Yes | Present | Enter bootloader |
| Watchdog (without flag) | ✅ Yes | Absent | Normal firmware start |
| External (RESET pin / Encoder) | ❓ Research | N/A | Normal firmware start* |
| Software Reset (Layer 2 key) | ✅ Yes** | Absent | Normal firmware start |

\* **Research Required**: Verify GPR preservation behavior for RESET pin  
\*\* Assumes watchdog-based software reset preserves GPR

**Key Distinction:**
- **Bootloader Entry (Fn+Ctrl+Ctrl)**: Sets boot flag → enters firmware update mode
- **Software Reset (Layer 2)**: Does NOT set boot flag → restarts firmware normally

### 4.5 Bootloader State Machine

**Boot Flow:**
```
Power On / Reset
    ↓
Bootloader Starts
    ↓
Check GPR for BOOT_MAGIC
    ↓
   [Magic Present?]
    ↓              ↓
   Yes            No
    ↓              ↓
Stay in         Jump to
Bootloader      Application
(USB HID)       (Keyboard)
```

**User-Visible Behavior:**

| Scenario | Boot Flag Set? | Result |
|----------|----------------|--------|
| Normal power-on | No | Keyboard starts immediately |
| Fn+LeftCtrl+RightCtrl pressed | Yes | Enter bootloader (FW update mode) |
| Layer 2 software reset key | No | Firmware restarts normally |
| Encoder button (RESET pin) | No* | Firmware restarts normally |
| UPDI programming | N/A | Direct flash write |

\* Assumes no boot flag set; research GPR behavior on RESET pin

### 4.6 Bootloader-Application Communication

**Fuse Configuration:**
- Reserve bootloader section in flash (research optimal size - maximize app space)
- Protect bootloader from application overwrites
- Never disable UPDI

**Jump Mechanism:**
```c
#define BOOTLOADER_ADDR 0x0000  // Adjust per fuse settings

void jump_to_bootloader(void) {
    cli();  // Disable interrupts
    ((void (*)(void))BOOTLOADER_ADDR)();
}
```

### 4.7 USB Enumeration Behavior

**From Host Perspective:**

1. **Normal Operation**: Device enumerates as "AmberClick90" keyboard
2. **Bootloader Entry (Fn+Ctrl+Ctrl)**: 
   - Keyboard disconnects
   - New HID device appears (bootloader)
   - Research: Consider using different device name (e.g., "AmberClick90 FW Update Mode")
3. **Firmware Update**: Host tool sends firmware via HID reports
4. **Completion**: Device resets, keyboard re-enumerates
5. **Software Reset (Layer 2)**: 
   - Brief USB disconnect/reconnect
   - Device re-enumerates as "AmberClick90" keyboard (same device)

---

## 5. Component Selection

### 5.1 Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **MCU** | AVR64DU32 | 1 | Use internal oscillator |
| **USB Connector** | USB Mini-B, through-hole | 1 | Full-Speed capable |
| **UPDI Header** | 3-4 pin header | 1 | See datasheet for pinout |
| **USB Series Resistors** | 22-33Ω, 0805 | 2 | For D+, D- |
| **USB Bulk Cap** | 10µF max, near connector | 1 | Per USB specification |
| **VDD Decoupling Caps** | 100nF, 0805 | Per VDD pin | Near each VDD pin |
| **VUSB Decoupling Cap** | 100nF, 0805 | 1 | Near VUSB pin |
| **Polyfuse** | 500mA, resettable | 1 | Between VBUS and VDD |
| **ESD Protection** | Low-cap USB diode array | 1 | Optional |
| **Switches** | Matias Click | 90 | Alps-compatible switches |
| **Keycaps** | Alps-compatible keycap set | 90 | See matias.ca/switches/click/ |
| **Diodes** | 1N4148 or equivalent | 90 | SMD or TH - see Section 14 |
| **Rotary Encoder** | ALPS EC11 with switch | 1 | With detent clicks |
| **Power LED** | 3mm amber, diffused, TH | 1 | Fixed brightness |
| **Indicator LEDs** | 3mm amber, diffused, TH | 2 | PWM-controlled |
| **LED Resistors** | TBD (see calculations) | 3 | Near LEDs |

### 5.2 LED Resistor Calculations

#### Power LED
- **Target**: Dim but visible (0.3-0.5mA typical)
- **Formula**: R = (VDD - Vf) / I = (5V - 2.0V) / 0.0005A ≈ 6kΩ
- **Selection Method**: Test in very dim environment to ensure non-annoying brightness

#### Indicator LEDs (PWM)
- **Target**: 5mA at 100% duty cycle (bright indoor visibility)
- **Formula**: R = (3.3V - 2.0V) / 0.005A = 260Ω
- **Standard Value**: 270Ω
- **Power**: P = I² × R = (0.005)² × 270 ≈ 7mW
- **Resistor Placement**: Near LED, not MCU (reduces EMI, easier rework)

---

## 6. PCB Design Guidelines

### 6.1 Physical Layout

**Component Placement:**
- USB connector: Top edge, aligned with Fn key
- MCU: As close to USB connector as possible (without compromising LED, switch, and encoder placement)
- Rotary encoder: Top-left corner
- Three LEDs: Horizontal arrangement near top edge
- UPDI header: Top edge (easier case design) or alternate edge for cleaner routing
- Switches: Standard keyboard grid layout

**Silkscreen:**
- Project name: "AmberClick90"
- Trim designation: "(AVRDU.Enc.Mat)"
- Typographic logo design (TBD)
- Minimalist, monochrome aesthetic
- Technical/instrument-like appearance

### 6.2 Design Specifications

- **PCB**: 2-layer, black solder mask
- **Case**: Minimal, grey 3D-printed (research filament grey color options - see Section 14)
- **Aesthetic**: Minimalist, technical

### 6.3 Net Naming Convention

**Power:**
- VDD (5V)
- VUSB (3.3V internal)
- GND

**Rows:**
- ROW_0 through ROW_9

**Columns:**
- COL_0 through COL_8

**Encoder:**
- ENC_A, ENC_B
- RESET (encoder switch)

**LEDs:**
- LED_PWR (power LED)
- LED_IND_A, LED_IND_B (indicators)

**USB:**
- USB_DP, USB_DM
- VBUS

**Programming:**
- UPDI
- RESET

### 6.4 Critical Routing

**USB Differential Pair:**
- Keep D+ and D- as short, matched-length traces
- Minimize stubs
- Avoid vias if possible
- Route as differential pair (typical impedance: 90Ω)

**Power:**
- Wide traces for VDD/GND
- Single ground plane (no separate analog ground)
- Star-ground from USB connector
- Polyfuse between VBUS and VDD

**Matrix:**
- Keep row/column traces away from USB lines
- Group by port for easier scanning

---

## 7. Firmware Development Roadmap

### 7.1 Initial Development Tasks

1. **Research Phase**
   - Study QMK ATmega32U4 implementation
   - Review DXCore for AVR DU specifics:
     - Pin mapping
     - Clock configuration
     - Fuse settings
     - Startup code
     - Linker scripts
     - UPDI flashing methods
   - Research TinyUSB integration
   - Define NKRO HID descriptor

2. **HAL Development**
   - GPIO abstraction (port-based access)
   - TCA timer setup for PWM
   - Watchdog configuration
   - Clock initialization

3. **Core Firmware**
   - Matrix scanning loop
   - Debouncing algorithm
   - Layer switching logic (2 main + 1 system layer)
   - Macro engine

4. **USB Stack**
   - TinyUSB integration
   - HID keyboard descriptor (NKRO)
   - USB enumeration
   - Device strings

5. **Bootloader**
   - HID bootloader implementation
   - GPR boot flag mechanism
   - Flash write routines
   - Application jump logic

6. **Development Environment**
   - Start firmware development using AVR64DU32 Nano dev board
   - Test basic functionality before PCB fabrication

### 7.2 Questions for LLM Assistant

**HAL & Architecture:**
- Sketch minimal HAL API for AVR64DU32
- Show TinyUSB HID report descriptor for NKRO
- Design HID bootloader strategy
- Provide bootloader entry mechanism (key combo vs reset)
- Show TinyUSB-compatible HID descriptor split (bootloader vs app)

**Matrix Scanning:**
- Show minimal scan loop for exact pin map (10 rows, 9 columns)
- Compare row-driven vs column-driven scanning
- Help decide pull-ups vs pull-downs
- Walk through diode orientation trade-offs

**Encoder Integration:**
- Sketch robust quadrature decoder
- Design Alt-Tab "smart encoder" behavior
- Suggest GPIO + pull-up configuration
- Show integration into matrix scan loop
- Show firmware logic for decoding encoders

**LED Control:**
- Suggest exact TCA PWM timer setup for PA4, PA5
- Choose PWM frequency that won't alias with matrix scanning
- Show exact TCA PWM timer config for AVR64DU32
- Recommend brightness step sizes
- Add cheap gamma correction
- Ensure PWM doesn't interfere with USB timing

**Power Analysis:**
- Check worst-case power consumption with everything on
- Estimate average current draw from USB
- Estimate maximum current draw (all LEDs + matrix)

**Bootloader Details:**
- Draft clean bootloader.c module
- Show minimal HID bootloader state machine
- Explain correct RESET button wiring (per datasheet)

**Clock & Fuses:**
- Research correct fuse settings (never disable UPDI)
- Select appropriate clock speed
- Evaluate sleep + wake-on-interrupt approach for power saving

---

## 8. Firmware Configuration Details

### 8.1 Clock Configuration

- **Primary Clock**: 24 MHz internal oscillator
- **USB Clock**: Derived from internal oscillator with USB clock recovery
- **Sleep Mode**: Evaluate wake-on-interrupt for power efficiency (research required)

### 8.2 Fuse Settings (Critical)

**Must Research:**
- Bootloader section size and location (maximize application firmware space)
- UPDI enable (NEVER disable)
- Brown-out detection level
- Watchdog configuration
- Reset pin configuration (tied to encoder switch)

### 8.3 Matrix Scanning Parameters

**Configuration:**
- Scan rate: 1-2 kHz typical
- Debounce time: 5-20ms (tune during testing)
- Pull configuration: TBD (pull-up vs pull-down)
- Diode orientation: TBD (COL2ROW vs ROW2COL)

**Port Optimization:**
- Rows grouped on single port where possible → single register read
- Columns distributed for routing convenience
- Minimize port switching during scan

### 8.4 PWM Configuration

**Timer Selection:**
- Use TCA (Timer Counter A) for PWM
- Single timer can drive both PWM channels (PA4, PA5)

**PWM Parameters:**
- Frequency: 1-2 kHz (avoid aliasing with matrix scan)
- Resolution: 8-bit (0-255)
- Brightness steps: 16-32 levels recommended
- Gamma correction: Optional, low-cost implementation

**Brightness Control:**
```c
// Default stored in Flash
#define LED_BRIGHTNESS_DEFAULT 128

// Runtime variable in SRAM
uint8_t led_brightness_a;
uint8_t led_brightness_b;

// Initialize at startup
led_brightness_a = LED_BRIGHTNESS_DEFAULT;
led_brightness_b = LED_BRIGHTNESS_DEFAULT;
```

### 8.5 Encoder Configuration

**Decoder Type:**
- State machine (preferred) or table-driven
- Handles mechanical bounce
- Integrates into main scan loop

**Sampling:**
- Poll during matrix scan (1-2 kHz)
- Or use pin-change interrupt (optional)

**Action Mapping (Example):**
```c
// Stored in Flash
const encoder_action_t encoder_map[] = {
    [LAYER_0] = { .cw = MACRO_VOL_UP,    .ccw = MACRO_VOL_DOWN },
    [LAYER_1] = { .cw = MACRO_ALT_TAB,   .ccw = MACRO_ALT_SHIFT_TAB }
};



## Prompt:
12/25/2025, 8:13:33 PM

Continue



## Response:
12/25/2025, 8:14:32 PM

```

---

## 9. Design Philosophy & Best Practices

### 9.1 Memory Management

**Flash Usage:**
- Store all constant data (keymaps, macros, descriptors)
- No runtime writes (except via bootloader)
- Maximize utilization of 64KB capacity
- Research bootloader section options to maximize application space
- Design allows scaling to future AVR-DU variants with >64KB flash

**SRAM Usage:**
- Minimize static allocation
- Use for runtime state only
- Monitor stack usage carefully
- Design allows scaling to future AVR-DU variants with >8KB SRAM

### 9.2 Code Quality

**Style:**
- Direct register access
- Minimal abstraction where performance critical
- Clean HAL for portability
- Well-commented for maintainability

**Portability:**
- Borrow concepts from QMK and DXCore
- Adapt for AVR DU specifics
- Document deviations from reference implementations
- Moderate forward compatibility with future AVR-DU variants

**Target Platform:**
- Firmware developed specifically for AVR64DU32
- Best MCU in AVR-DU family currently available
- No backward compatibility concerns with smaller AVR-DU parts

### 9.3 User Experience

**Boot Time:**
- Instant firmware start (<100ms)
- Quick bootloader check (no delay)

**Responsiveness:**
- Low-latency matrix scanning
- Immediate key feedback
- Smooth encoder operation

**Reliability:**
- Robust debouncing
- No false triggering
- Stable USB enumeration

---

## 10. Testing & Validation

### 10.1 Hardware Testing

- [ ] USB enumeration on multiple hosts (Windows, macOS, Linux)
- [ ] All 90 keys register correctly
- [ ] Encoder rotation and detents
- [ ] Reset button (encoder push) - firmware restart
- [ ] All three LEDs illuminate
- [ ] PWM dimming works correctly
- [ ] UPDI programming successful
- [ ] Bootloader entry via Fn+LeftCtrl+RightCtrl
- [ ] Software reset key (Layer 2) - firmware restart only

### 10.2 Firmware Testing

- [ ] Matrix scanning accuracy (no ghost keys)
- [ ] Debouncing effectiveness
- [ ] Layer switching (2 main + 1 system layer)
- [ ] Macro execution
- [ ] NKRO verification (6+ simultaneous keys)
- [ ] Encoder decoding accuracy
- [ ] LED brightness control
- [ ] Bootloader mode entry (Fn+Ctrl+Ctrl → bootloader)
- [ ] Software reset (Layer 2 → firmware restart, not bootloader)
- [ ] Firmware update via USB

### 10.3 Power Testing

- [ ] USB current draw (idle)
- [ ] USB current draw (all keys pressed)
- [ ] USB current draw (all LEDs at 100%)
- [ ] Verify ≤500mA total (USB spec compliance)
- [ ] Polyfuse operation verification

---

## 11. Future Enhancements (Optional)

**Pin-Saving Techniques (if needed):**
- Share LEDs with matrix columns (LED active only when column driven)
- Encoder pins as matrix keys (careful decoding required)
- Note: Current design has sufficient GPIO, these are optional

**Advanced Features:**
- VIA/VIAL support for runtime keymap editing
- RGB underglow (if desired)
- Audio output (piezo buzzer)
- Additional encoders
- OLED display

**Platform Scaling:**
- Firmware can be adapted for future AVR-DU variants with:
  - More than 64 KB Flash
  - More than 8 KB SRAM
  - More than 32 pins

---

## 12. References & Resources

**Datasheets:**
- [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)
- [ALPS EC11 Encoder Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)
- [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)
- [Matias Click Switch & Keycaps](https://matias.ca/switches/click/)
- USB 2.0 specification

**Software:**
- QMK Firmware (reference implementation)
- DXCore (AVR DU support)
- TinyUSB (USB stack)
- avr-gcc documentation

**Standards:**
- USB HID Usage Tables
- USB 2.0 Full-Speed specification
- Keyboard scan code standards

---

## 13. Typographic Logo Design

**Requirements:**
- Simple, silkscreen-friendly design
- Minimalist aesthetic
- Technical/instrument-like appearance
- Monochrome (black PCB, white silkscreen)
- Incorporates "AmberClick90" or "AC90" branding

**Design Status**: TBD

---

## 14. Outstanding Research Questions

### 14.1 Hardware Questions

1. **3D Printed Case**
   - What grey filament colors are available?
   - Which grey shade best matches minimalist aesthetic?
   - PLA, PETG, or ABS for case material?

2. **Matias Switches & Keycaps**
   - ✅ **Resolved**: Matias switches use Alps mount
   - ✅ **Resolved**: Alps-compatible keycaps available at [matias.ca/switches/click/](https://matias.ca/switches/click/)
   - Verify PCB footprint interchangeability between Matias Click and Matias Quiet Click variants
   - Research availability and pricing of Alps-compatible keycap sets

3. **Matrix Diodes**
   - SMD vs through-hole diodes for switch matrix?
   - If SMD: SOD-123 or SOD-323 package?
   - If TH: standard DO-35 package?
   - PCB routing considerations for each option

4. **UPDI Programming**
   - UPDI programmer dongle options (SerialUPDI vs hardware programmer)
   - UPDI software tools (avrdude support for AVR DU?)
   - UPDI header pinout (3-pin vs 4-pin configuration)

5. **USB Bootloader Tools**
   - QMK Toolbox compatibility?
   - Custom flashing tool requirements?
   - Host-side software for HID bootloader

### 14.2 Firmware Questions

6. **Reset Behavior**
   - Do GPR values survive RESET pin assertion? (datasheet suggests yes - verify)
   - Do GPR values survive direct software reset? (datasheet suggests yes - verify)
   - Testing required to confirm GPR preservation across reset types

7. **Bootloader Configuration**
   - What are the bootloader section size options in fuses?
   - How to maximize application firmware space?
   - Optimal bootloader section configuration for 64KB flash

8. **USB Device Naming**
   - Should bootloader present different HID device name?
   - "AmberClick90" vs "AmberClick90 FW Update Mode"
   - User experience implications of name change

9. **Clock & Power**
   - Optimal clock speed settings and fuse configuration
   - Sleep mode implementation for power savings
   - Wake-on-interrupt approach feasibility

10. **Matrix Scanning**
    - Row-driven vs column-driven scanning performance comparison
    - Pull-up vs pull-down resistor configuration
    - Diode orientation (COL2ROW vs ROW2COL) trade-offs

11. **NKRO Implementation**
    - HID descriptor format for full NKRO
    - USB report structure for 90-key NKRO
    - Compatibility considerations across operating systems

12. **USB HID Descriptor**
    - Setup requirements for custom HID descriptors
    - Split descriptor approach (bootloader vs application)
    - TinyUSB integration specifics

13. **DXCore Research**
    - Pin mapping conventions for AVR DU
    - Clock and fuse settings used by DXCore
    - Startup code and linker script templates
    - UPDI flashing methodology in DXCore

14. **PWM Timing**
    - TCA timer configuration for dual-channel PWM
    - PWM frequency selection to avoid matrix scan aliasing
    - Gamma correction implementation (lookup table vs calculation)

15. **Encoder Implementation**
    - Optimal quadrature decoder algorithm
    - "Smart" Alt-Tab behavior implementation details
    - Encoder state initialization to avoid phantom rotations on boot

### 14.3 Power & Safety Questions

16. **Current Draw Analysis**
    - Worst-case current consumption calculation
    - Average current draw during typical use
    - Peak current during all-keys-pressed + all-LEDs-on scenario
    - Polyfuse rating verification (500mA appropriate?)

### 14.4 PCB & Assembly Questions

17. **Component Sourcing**
    - AVR64DU32 availability and pricing
    - Matias switch availability (Click and Quiet Click)
    - Alps-compatible keycap sets (availability and pricing)
    - Through-hole vs SMD component cost comparison

18. **MCU Placement**
    - Optimal MCU location balancing:
      - Proximity to USB connector
      - LED placement constraints
      - Switch matrix routing
      - Encoder location
      - Overall PCB layout efficiency

---

## 15. Document Status

This specification represents the **current consolidated design** as of this revision and serves as:

- Complete reference for schematic and PCB design
- Firmware architecture guide
- Component selection reference
- Testing checklist
- Handoff document for future development
- Comprehensive list of outstanding research questions

**Key Design Decisions:**
- Firmware targets AVR64DU32 specifically (best AVR-DU currently available)
- Moderate forward compatibility with future larger AVR-DU variants
- Layer 2 software reset key restarts firmware (does NOT enter bootloader)
- Bootloader entry only via Fn+LeftCtrl+RightCtrl combination
- Matias Click switches with Alps-compatible keycaps

**Next Steps:**
1. Research outstanding questions (Section 14)
2. Finalize typographic logo design
3. Begin firmware development using AVR64DU32 Nano dev board
4. Create KiCad schematic
5. Complete PCB layout
6. Implement and test bootloader

---

*Document Version: 2.1*  
*Last Updated: December 25, 2025*  
*Project: AmberClick90 (AVRDU.Enc.Mat)*



## Prompt:
12/25/2025, 8:15:38 PM

Comment on next steps including their ordering.



## Response:
12/25/2025, 8:16:40 PM

## Next Steps: Commentary and Ordering

### Phase 1: Research & Knowledge Gathering (Weeks 1-2)
**Priority: Critical - Must complete before design work**

1. **Resolve Outstanding Hardware Questions (Section 14.1-14.4)**
   - *Rationale*: Design decisions affect schematic, PCB layout, and firmware architecture
   - *Key Items*:
     - Diode selection (SMD vs TH) - affects PCB footprint design
     - UPDI header pinout - must be correct for programming
     - Matias Click vs Quiet Click footprint compatibility - affects switch selection flexibility
     - Grey filament research - can be done in parallel with other work
   
2. **DXCore Deep Dive**
   - *Rationale*: Provides proven baseline for AVR64DU32 usage
   - *Focus Areas*:
     - Fuse configurations (especially bootloader section sizing)
     - Startup code and linker scripts (needed for bare-metal development)
     - UPDI flashing workflow
   - *Why First*: This information directly influences firmware architecture decisions

3. **TinyUSB + NKRO Research**
   - *Rationale*: USB stack is core to both bootloader and application
   - *Deliverables*:
     - NKRO HID descriptor format for 90 keys
     - Understanding of TinyUSB integration with bare-metal AVR
     - Split descriptor strategy (bootloader vs application)
   - *Why Early*: Affects memory layout planning and bootloader size estimates

### Phase 2: Firmware Development Environment Setup (Weeks 2-3)
**Priority: High - Enables parallel hardware and firmware development**

4. **Acquire AVR64DU32 Nano Dev Board**
   - *Rationale*: Essential for firmware development before PCB is ready
   - *Benefits*:
     - Test GPIO configuration
     - Validate USB enumeration
     - Develop and debug HAL
     - Test bootloader concept with GPR method
   - *Why Before PCB*: De-risks firmware by proving concepts early

5. **Setup Development Toolchain**
   - *Components*:
     - avr-gcc compiler chain
     - Geany configuration
     - UPDI programmer + SerialUPDI software
     - USB HID flashing tool research
   - *Why Now*: Required for any firmware work on dev board

6. **Minimal HAL Development on Dev Board**
   - *Start With*:
     - GPIO abstraction (port-based access)
     - TCA PWM setup (test with LEDs)
     - Basic USB enumeration with TinyUSB
   - *Rationale*: Proves toolchain works before investing in PCB
   - *Risk Mitigation*: Identifies any AVR64DU32-specific quirks early

### Phase 3: Schematic Design (Weeks 3-4)
**Priority: High - Blocks PCB layout**

7. **Create KiCad Schematic**
   - *Order of Schematic Pages*:
     1. Power (USB → polyfuse → VDD, decoupling)
     2. MCU (pin assignments, UPDI, RESET)
     3. USB interface (connector, series resistors, ESD protection)
     4. Key matrix (rows, columns, diodes - once diode type decided)
     5. Encoder (EC11 connections)
     6. LEDs (power LED + 2× PWM indicators with resistors)
   - *Why This Order*: Power first establishes voltage domains; MCU next locks pin assignments
   - *Checkpoint*: Peer review schematic before proceeding to layout

8. **Finalize Typographic Logo**
   - *Timing*: Can happen in parallel with schematic
   - *Requirement*: Must be complete before PCB layout for silkscreen
   - *Format*: Vector format suitable for KiCad import

### Phase 4: Parallel Firmware Development (Weeks 4-6)
**Priority: High - Can proceed during PCB design/fabrication**

9. **Bootloader Implementation on Dev Board**
   - *Components*:
     - GPR boot flag mechanism (test reset preservation)
     - Minimal HID bootloader
     - Flash write routines
     - Application jump logic
   - *Testing*:
     - Verify GPR survives watchdog reset
     - Test GPR behavior with RESET pin (research question #6)
     - Validate USB enumeration in bootloader mode
   - *Why Parallel*: Long lead time for PCB fabrication allows firmware work

10. **Core Keyboard Firmware on Dev Board**
    - *Phase 4a - Matrix Simulation*:
      - Use dev board GPIO to simulate small matrix (3×3)
      - Test debouncing algorithm
      - Validate scanning speed
    - *Phase 4b - USB HID Implementation*:
      - NKRO descriptor
      - Key press/release reporting
      - Layer switching logic
    - *Phase 4c - Encoder Emulation*:
      - Quadrature decoder with two buttons on dev board
      - Test "smart" Alt-Tab behavior
    - *Rationale*: Most firmware can be developed without physical keyboard

### Phase 5: PCB Design (Weeks 5-7)
**Priority: High - Critical path to hardware**

11. **PCB Layout in KiCad**
    - *Order of Operations*:
      1. Board outline and mounting holes
      2. USB connector placement (top edge, align with Fn key position)
      3. MCU placement (as close to USB as possible)
      4. Critical component placement (encoder, LEDs, UPDI header)
      5. USB differential pair routing (D+/D-, matched length, 90Ω impedance)
      6. Power routing (VDD, GND, wide traces, star ground)
      7. Matrix routing (rows and columns, diode placement)
      8. Remaining signal routing
      9. Ground plane pour
      10. Silkscreen (logo, labels, pin functions)
      11. Design rule check (DRC)
   - *Why This Order*: Critical signals first (USB), then power, then matrix
   - *Checkpoint*: Design review before ordering PCBs

12. **Component Placement Optimization**
    - *Balance*:
      - MCU near USB (minimize D+/D- trace length)
      - LEDs visible and aesthetically arranged
      - Encoder in accessible location (top-left)
      - Switch matrix on standard grid
   - *Iteration*: May require 2-3 layout iterations for optimal routing

### Phase 6: Fabrication & Assembly (Weeks 8-10)
**Priority: Medium - Waiting period allows firmware refinement**

13. **Order PCBs**
    - *Lead Time*: Typically 2-4 weeks depending on supplier
    - *During Fabrication*: Continue firmware development on dev board

14. **Order Components**
    - *Critical Long-Lead Items*:
      - AVR64DU32 (check stock)
      - Matias Click switches (90 units)
      - Alps-compatible keycaps (verify availability)
      - ALPS EC11 encoder
    - *Timing*: Order immediately after schematic finalized
    - *Buffer*: Order extras of small components (diodes, resistors, caps)

15. **PCB Assembly**
    - *Assembly Order*:
      1. SMD components (if any diodes are SMD)
      2. MCU (careful with orientation)
      3. Passive components (resistors, capacitors)
      4. USB connector
      5. UPDI header
      6. LEDs (last, to avoid mechanical stress)
      7. Switches (after PCB testing)
      8. Encoder (after PCB testing)
   - *Why This Order*: Test basic functionality before committing to switches

### Phase 7: 3D Case Design (Weeks 9-11)
**Priority: Low - Can wait for PCB to verify fit**

16. **Case Design**
    - *Wait For*: PCB fabrication complete to verify dimensions
    - *Design Process*:
      1. Measure actual PCB dimensions
      2. CAD model in Fusion 360 or OpenSCAD
      3. Test print of small section (verify tolerances)
      4. Full case print
      5. Fit test with assembled PCB
   - *Grey Filament*: Research and select during this phase
   - *Why Later*: PCB dimensions may differ slightly from design; case must fit actual PCB

### Phase 8: Hardware Testing & Integration (Weeks 11-12)
**Priority: Critical - Validation of entire system**

17. **Initial PCB Testing (Before Switch Assembly)**
    - *Test Sequence*:
      1. Visual inspection (shorts, solder bridges)
      2. Continuity testing (VDD, GND)
      3. Power-on test (measure voltages)
      4. UPDI programming test (flash LED blink test)
      5. USB enumeration test
      6. Basic GPIO test (toggle pins)
    - *Why Before Switches*: Easier to debug and rework bare PCB

18. **Bootloader Installation via UPDI**
    - *Flash bootloader using UPDI programmer*
    - *Test bootloader entry mechanism*
    - *Verify USB HID enumeration in bootloader mode*

19. **Switch Assembly**
    - *Only After*: Basic PCB functionality confirmed
    - *Process*:
      1. Install switches in PCB
      2. Test continuity of each switch position
      3. Install encoder
      4. Test encoder functionality

20. **Full Firmware Integration Testing**
    - *Test Plan* (from Section 10):
      - All 90 keys register correctly
      - NKRO with 10+ simultaneous keys
      - Layer switching
      - Encoder rotation and detents
      - LED brightness control
      - Bootloader entry (Fn+LeftCtrl+RightCtrl)
      - Software reset (Layer 2)
      - Macro execution

### Phase 9: Refinement & Documentation (Weeks 13+)
**Priority: Medium - Quality of life improvements**

21. **Firmware Tuning**
    - Debounce timing optimization
    - LED brightness curve adjustment
    - Encoder sensitivity tuning
    - Layer behavior refinement

22. **Case Finalization**
    - Adjust case design based on assembly experience
    - Final print with selected grey filament
    - Fit and finish improvements

23. **Documentation**
    - Build guide
    - Firmware flashing instructions
    - Troubleshooting guide
    - Component sourcing guide

---

## Critical Path Analysis

**Longest Lead Items** (determine project timeline):
1. **PCB fabrication** (2-4 weeks) - can't be shortened
2. **Component availability** (especially Matias switches, keycaps) - order early
3. **Firmware development** (4-6 weeks) - can be parallelized with hardware

**Optimal Strategy**:
- **Weeks 1-3**: Research + dev board setup + schematic → firmware development starts
- **Weeks 3-5**: PCB layout + component ordering → overlap with firmware dev
- **Weeks 5-8**: PCB fabrication (waiting period) → continue firmware development on dev board
- **Weeks 8-10**: Assembly + testing → integrate firmware with hardware
- **Weeks 10+**: Refinement, case design/printing, documentation

**Total Estimated Timeline**: 10-13 weeks from start to fully functional keyboard

**Key Parallelization Opportunities**:
- Firmware development on dev board during PCB fabrication
- Logo design during schematic work
- Component ordering immediately after schematic finalization
- Grey filament research as background task
- Case design after PCB dimensions confirmed but before final assembly

**Risk Mitigation**:
- Dev board work de-risks firmware before PCB investment
- PCB testing before switch assembly prevents wasted components
- Early component ordering prevents delays from stock issues
- Design reviews at schematic and layout stages catch errors early



## Prompt:
12/25/2025, 8:25:26 PM

are there inconsistencies or contradictions in the last version of the technical specification document, assuming i adopt the order of the steps you just suggested?



## Response:
12/25/2025, 8:26:32 PM

# Inconsistencies and Contradictions Analysis

After reviewing the technical specification document against the proposed next steps, I've identified the following issues:

## 1. **Major Inconsistency: Firmware Development Prerequisites**

**Contradiction:**
- **Section 7.1** states: "Research Phase" should happen first, including studying QMK, DXCore, and TinyUSB
- **Next Steps Phase 2** suggests: "Minimal HAL Development on Dev Board" in weeks 2-3
- **Problem**: You can't develop HAL without completing the research phase first

**Resolution Needed:**
- Research must be substantially complete before dev board work begins
- Some research (like grey filament) can be deferred, but DXCore/TinyUSB research is blocking

---

## 2. **Minor Contradiction: PCB Testing Sequence**

**Section 10.1 states:**
```
- [ ] Bootloader entry via Fn+LeftCtrl+RightCtrl
- [ ] Software reset key (Layer 2) - firmware restart only
```

**But Next Steps Phase 8 (Step 17) says:**
"Initial PCB Testing (Before Switch Assembly)"

**Problem**: Can't test key combinations without switches installed, but the testing checklist implies these should be tested.

**Resolution Needed:**
- Clarify that Section 10.1 tests are performed AFTER switch assembly
- Step 17 should only test basic electrical functionality (power, USB, GPIO)
- Add a Step 20.5 for "Full Keyboard Function Testing" with the Section 10.1 checklist

---

## 3. **Timing Inconsistency: Logo Design**

**Section 6.1** places logo under silkscreen requirements (implies needed for PCB)

**Section 13** says: "Design Status: TBD"

**Next Steps Phase 3, Step 8** says: "Finalize Typographic Logo... Must be complete before PCB layout"

**Problem**: PCB layout starts Week 5, but logo is still "TBD" in specification

**Resolution Needed:**
- Move logo design to Phase 2 or early Phase 3
- Update Section 13 to clarify logo must be designed before PCB layout begins
- Or note that PCB layout can proceed with placeholder, logo added before final design review

---

## 4. **Ambiguity: Component Ordering Timing**

**Next Steps Phase 6, Step 14** says:
"Order Components... Timing: Order immediately after schematic finalized"

**But also says:**
"Critical Long-Lead Items... check stock"

**Problem**: 
- Schematic is finalized Week 4
- PCB layout happens Weeks 5-7
- What if PCB layout reveals footprint issues requiring component changes?

**Resolution Needed:**
- Separate component ordering into two phases:
  - **Phase 1**: Order AVR64DU32, standard passives (after schematic)
  - **Phase 2**: Order switches, keycaps, encoder (after PCB layout validated)
- Or add a buffer: "Order components after PCB layout DRC passes"

---

## 5. **Contradiction: Diode Selection Timing**

**Section 14.1, Question 3** says:
"SMD vs through-hole diodes for switch matrix?" - **Outstanding research question**

**Next Steps Phase 1** says:
"Diode selection (SMD vs TH) - affects PCB footprint design" - **Must resolve before design**

**Next Steps Phase 3, Step 7** says:
"Key matrix (rows, columns, diodes - **once diode type decided**)"

**Problem**: This is correctly identified as blocking, but the document doesn't emphasize urgency

**Resolution Needed:**
- Move diode decision to "Week 1 Priority" in Section 14
- Add to Phase 1 critical path items
- Make explicit: "MUST decide by end of Week 1 or schematic is blocked"

---

## 6. **Minor Inconsistency: Reset Behavior Research**

**Section 4.4** shows a table with:
```
| External (RESET pin / Encoder) | ❓ Research | N/A | Normal firmware start* |
| Software Reset (Layer 2 key) | ✅ Yes** | Absent | Normal firmware start |
```

**Footnote says**: "Assumes watchdog-based software reset preserves GPR"

**Problem**: 
- Layer 2 software reset is marked as "✅ Yes**" (GPR preserved)
- But there's an assumption that needs verification
- This contradicts the confident checkmark

**Resolution Needed:**
- Change Layer 2 software reset to "❓ Research" until verified
- Or remove the assumption footnote if we're confident about watchdog behavior
- Make consistent: either both are research items, or both are verified

---

## 7. **Resource Availability Assumption**

**Section 3.1** states:
"Development Board: AVR64DU32 Nano dev board (for initial firmware development)"

**Next Steps Phase 2, Step 4** says:
"Acquire AVR64DU32 Nano dev board"

**Problem**: Document assumes this board exists and is readily available

**Missing Information**:
- Where to purchase?
- Lead time for acquisition?
- What if unavailable?
- Alternative: bare AVR64DU32 + breadboard setup?

**Resolution Needed:**
- Add to Section 14 Outstanding Questions: "AVR64DU32 Nano availability and sourcing"
- Or specify alternative development approach if unavailable
- Add to Phase 1: "Verify dev board availability before committing to timeline"

---

## 8. **Forward Compatibility Claims vs Implementation**

**Section 2.1** states:
"Design Philosophy: Moderate flexibility for forward compatibility with future AVR-DU variants"

**Section 9.2** states:
"Target Platform: Firmware developed specifically for AVR64DU32"

**Problem**: These statements appear contradictory

**What "Moderate Flexibility" Actually Means** (needs clarification):
- Code structure allows scaling (not hardcoded limits)?
- Conditional compilation for different variants?
- Just architectural cleanliness (no actual multi-variant support)?

**Resolution Needed:**
- Clarify in Section 2.1 what "moderate flexibility" means in practice
- Add note: "No actual support for other variants; code structure allows future porting"
- Or remove forward compatibility claims if not implementing any flexibility

---

## 9. **Bootloader Size Estimates vs Fuse Configuration**

**Section 3.3** states:
"HID Bootloader (4-6 KB, ~6-9%)"
"Research: maximize app firmware space"

**Section 4.6** states:
"Reserve bootloader section via fuses"

**Section 14.2, Question 7** asks:
"What are the bootloader section size options in fuses?"

**Problem**: Bootloader size estimate given, but actual fuse options unknown

**Potential Issues**:
- AVR fuses typically use fixed section sizes (512B, 1KB, 2KB, 4KB, 8KB)
- 4-6KB estimate might not align with available fuse settings
- Might need to reserve 8KB section for a 6KB bootloader (waste)

**Resolution Needed:**
- Move bootloader section research to Week 1 (Phase 1)
- Update Section 3.3 estimates based on actual fuse options
- Show memory map with actual fuse-defined boundaries, not estimates

---

## 10. **PWM Frequency Selection Dependencies**

**Section 2.6** states:
"PWM Configuration: Frequency: ≥500 Hz, preferably 1-2 kHz"

**Section 8.4** states:
"PWM Parameters: Frequency: 1-2 kHz (avoid aliasing with matrix scan)"

**Section 8.3** states:
"Scan rate: 1-2 kHz typical"

**Problem**: PWM and matrix scan at same frequency = potential aliasing!

**Resolution Needed**:
- Change PWM target to avoid overlap: "PWM: 2-3 kHz" or "PWM: 500-800 Hz"
- Make explicit relationship: "PWM frequency must be >2× or <0.5× matrix scan rate"
- Add to Section 7.2 LLM questions: "Verify PWM frequency doesn't alias with 1-2 kHz matrix scan"

---

## 11. **Case Design Timing vs PCB Verification**

**Next Steps Phase 7, Step 16** says:
"Case Design... Wait For: PCB fabrication complete to verify dimensions"

**But also says**:
"Weeks 9-11" for case design

**Timeline shows**:
- Week 8-10: Assembly + testing
- Week 9-11: Case design

**Problem**: Case design starts Week 9, but PCB assembly happens Week 8-10

**Questions**:
- Can case design start with just bare PCB (Week 8)?
- Or must wait for full assembly including switches (Week 10)?
- Switch height critical for case design

**Resolution Needed**:
- Clarify: "Case design starts after bare PCB dimensions verified (Week 8)"
- Add: "Case refinement after switch assembly confirms heights (Week 10)"
- Or delay case design start to Week 10 if switch height is critical dimension

---

## 12. **Encoder Push Switch Function Clarity**

**Section 2.5** states clearly:
"Integrated push switch → RESET pin (hardware reset only)"

**Section 4.2** correctly states:
"Encoder button (RESET pin) → firmware restart"

**Section 10.1** test checklist says:
"Reset button (encoder push) - firmware restart"

**This is actually CONSISTENT**, but could be clearer about **what "firmware restart" means**:

**Ambiguity**:
- Does encoder push → always firmware restart (never bootloader)?
- Or does it depend on GPR state from previous operation?

**Resolution Needed**:
- Add explicit note in Section 2.5: "Encoder push does NOT enter bootloader; always starts firmware normally (unless GPR flag set by software)"
- Clarify in Section 4.4 footnote what happens if GPR preserved across RESET pin

---

## Summary of Required Changes

### Critical (Block Progress):
1. ✅ **Diode selection** - Must resolve Week 1
2. ✅ **Bootloader fuse options** - Must research Week 1 
3. ✅ **PWM/matrix scan frequency conflict** - Fix specification
4. ✅ **Dev board availability** - Verify Week 1

### High Priority (Affect Timeline):
5. ✅ **Component ordering split** - Refine strategy
6. ✅ **Logo design timing** - Move to Week 2-3
7. ✅ **Research phase completion** - Must finish before HAL work
8. ✅ **GPR reset behavior** - Mark as research until verified

### Medium Priority (Documentation Quality):
9. ✅ **Testing sequence** - Separate bare PCB vs full assembly tests
10. ✅ **Forward compatibility claims** - Clarify what this means
11. ✅ **Case design timing** - Clarify bare PCB vs full assembly dependency

### Low Priority (Minor Clarifications):
12. ✅ **Encoder reset behavior** - Add explicit note about bootloader vs firmware restart

**Recommendation**: Update the specification document to resolve items 1-4 before proceeding with any design work, and address items 5-8 during Phase 1 (Weeks 1-2).



## Prompt:
12/25/2025, 10:12:42 PM

Bootloader code section size setting via fuse is very granular and can vary in increments of 512 bytes from 0 to 64kb.



## Response:
12/25/2025, 10:13:25 PM

Thank you for that clarification! That's excellent news - the granular 512-byte increments give us precise control over the bootloader section size.

Let me update the analysis:

---

## Updated: Inconsistency #9 - RESOLVED

**Section 3.3** states:
"HID Bootloader (4-6 KB, ~6-9%)"

**Section 14.2, Question 7** asks:
"What are the bootloader section size options in fuses?"

**Resolution:**
✅ **RESOLVED** - Fuses allow 512-byte increments from 0 to 64KB

**Implications:**
- 4KB bootloader → set fuse to 4KB (exactly)
- 6KB bootloader → set fuse to 6KB (exactly)
- No wasted flash space due to fixed section sizes
- Can optimize bootloader size and adjust fuse accordingly

**Updated Memory Map** (example with 6KB bootloader):
```
|------------------------| 0x0000
| HID Bootloader         |
| (6 KB = 0x1800 bytes)  |
| Fuse: BOOTSIZE=0x0C    |
|------------------------| 0x1800
| Application Firmware   |
| (58 KB available)      |
|                        |
|                        |
|                        |
|------------------------| 0x10000 (64KB)
```

**Action Items:**
1. Remove Question 7 from Section 14.2 (now answered)
2. Update Section 3.3 to note: "Bootloader section configurable in 512-byte increments via BOOTSIZE fuse"
3. Update Section 4.6 to specify: "Set BOOTSIZE fuse to actual bootloader size (rounded up to nearest 512 bytes)"
4. Add to Section 8.2 fuse settings: "BOOTSIZE = (bootloader_size_bytes / 512), rounded up"

---

## Revised Critical Issues List

With bootloader sizing resolved, here's the **updated critical issues** that remain:

### Critical (Block Progress) - Must Resolve Week 1:
1. ✅ **Diode selection (SMD vs TH)** - Affects PCB footprint design
2. ✅ **PWM/matrix scan frequency conflict** - 1-2 kHz for both = potential aliasing
3. ✅ **Dev board availability** - Verify AVR64DU32 Nano sourcing

### High Priority (Affect Timeline) - Resolve Weeks 1-2:
4. ✅ **Component ordering strategy** - Split into two phases to avoid committing before PCB validation
5. ✅ **Logo design timing** - Must complete by Week 3 (before PCB layout)
6. ✅ **Research phase dependencies** - DXCore/TinyUSB research must complete before HAL development
7. ✅ **GPR reset behavior verification** - Test on dev board to confirm RESET pin and software reset behavior

### Medium Priority (Documentation Quality) - Address During Development:
8. ✅ **Testing sequence clarification** - Separate Section 10.1 into pre-switch and post-switch tests
9. ✅ **Forward compatibility claims** - Clarify what "moderate flexibility" means in practice
10. ✅ **Case design timing** - Specify bare PCB (Week 8) vs full assembly (Week 10) dependencies

### Low Priority (Minor Clarifications):
11. ✅ **Encoder reset behavior note** - Add explicit statement that encoder push never enters bootloader

---

## Updated Section 14.2 - Outstanding Questions

**Question 7 should be updated to:**

~~7. **Bootloader Configuration**~~
   ~~- What are the bootloader section size options in fuses?~~
   ~~- How to maximize application firmware space?~~
   ~~- Optimal bootloader section configuration for 64KB flash~~

**Replaced with:**

7. **Bootloader Configuration** ✅ **PARTIALLY RESOLVED**
   - ✅ **RESOLVED**: Fuse BOOTSIZE allows 512-byte increments (0 to 64KB)
   - ✅ **RESOLVED**: Set BOOTSIZE = (bootloader_bytes / 512) rounded up
   - ❓ **REMAINING**: What is actual compiled bootloader size for our minimal HID implementation?
   - ❓ **REMAINING**: Reserve 4KB, 4.5KB, 5KB, 5.5KB, or 6KB? (Determine during bootloader development)

---

## Recommended Specification Updates

### 1. Update Section 3.3 Memory Layout:

**Current:**
```
|------------------------| 0x0000
| HID Bootloader         |
| (4-6 KB, ~6-9%)        |
| Research: maximize     |
| app firmware space     |
|------------------------|
```

**Revised:**
```
|------------------------| 0x0000
| HID Bootloader         |
| (Target: 4-6 KB)       |
| Fuse: BOOTSIZE         |
| (512-byte increments)  |
|------------------------| 0x1000 to 0x1800 (TBD)
| Application Firmware   |
| (60-58 KB available)   |
| Size determined by     |
| BOOTSIZE fuse setting  |
|------------------------|
| Unused                 |
| (36-42 KB free)        |
|------------------------| 0x10000 (64KB)
```

### 2. Update Section 4.6 Bootloader-Application Communication:

**Add after "Fuse Configuration" bullet:**
- Reserve bootloader section via BOOTSIZE fuse (512-byte increments)
- Example: 5.5KB bootloader → BOOTSIZE = 0x0B (5.5KB = 5632 bytes / 512 = 11 = 0x0B)
- Maximize application space by setting BOOTSIZE to actual bootloader size + safety margin (~512 bytes)
- Protect bootloader from application overwrites
- Never disable UPDI

### 3. Update Section 8.2 Fuse Settings:

**Add specific fuse:**
```
**Must Research:**
- BOOTSIZE: Set to (compiled_bootloader_size / 512) rounded up
  - Typical: 0x08 to 0x0C (4KB to 6KB)
  - Determined after bootloader compilation
  - Leave ~512-byte safety margin above actual size
- UPDI enable (NEVER disable)
- Brown-out detection level
- Watchdog configuration
- Reset pin configuration (tied to encoder switch)
```

### 4. Update Next Steps Phase 9 (Step 21):

**Add sub-item:**
```
21. **Firmware Tuning**
    - Debounce timing optimization
    - LED brightness curve adjustment
    - Encoder sensitivity tuning
    - Layer behavior refinement
    - **Bootloader size optimization:**
      - Measure final compiled bootloader size
      - Adjust BOOTSIZE fuse to minimum safe value
      - Recalculate application space available
      - Update memory map documentation
```

---

## New Advantage: Iterative Bootloader Development

With 512-byte granularity, you can:

1. **Week 4-6**: Develop bootloader, initially reserve 8KB (BOOTSIZE = 0x10) for safety
2. **Week 9**: Optimize bootloader, reduce to 6KB (BOOTSIZE = 0x0C), gain 2KB app space
3. **Week 11**: Final optimization, reduce to 5KB (BOOTSIZE = 0x0A), gain another 1KB
4. **Update fuses** without changing code, just recompiling with different linker script

This flexibility is actually a **significant advantage** of AVR-DU over older AVRs with fixed bootloader sections!

---

Does this resolve the bootloader sizing concern adequately? The main remaining critical issues are now:
1. Diode selection (SMD vs TH)
2. PWM/matrix frequency conflict
3. Dev board availability verification



## Prompt:
12/25/2025, 10:18:19 PM

use smd diode but pick a size that can easily be soldered by hand using a regular solder iron (and suggest a recommended tip for the hakko fx 888dx (common for diy).



## Response:
12/25/2025, 10:19:11 PM

# Updated: Diode Selection - RESOLVED

## SMD Diode Specification

**Selected Package: SOD-123**

### Rationale for SOD-123:
- **Hand-solderable**: Large enough for comfortable manual soldering
- **Size**: 3.5mm × 1.6mm × 1.1mm (comparable to 0805 resistor width)
- **Pad spacing**: ~2.7mm between pads (easy to work with)
- **Common availability**: Standard package, many suppliers
- **Cost-effective**: Widely manufactured
- **Not too large**: Still smaller than through-hole DO-35

### Recommended Diode Part:
- **Part Number**: 1N4148W (SOD-123 package)
- **Alternatives**: 1N4148WS, BAV16W, BAS16 (all SOD-123)
- **Specs**: 100V, 300mA, fast switching
- **Widely available**: Digikey, Mouser, LCSC, JLCPCB

### Hakko FX-888DX Soldering Iron Tip Recommendation

**Recommended Tip: T18-C2**
- **Type**: Conical tip
- **Size**: 2mm diameter
- **Best for**: SOD-123, 0805, and general SMD work
- **Why this tip**:
  - Point is large enough for good heat transfer
  - Small enough for precision on SOD-123 pads
  - Versatile for other keyboard components

**Alternative Tip: T18-D12**
- **Type**: Chisel tip
- **Size**: 1.2mm width
- **Best for**: Drag soldering, SOD-123, 0805
- **Why this tip**:
  - Flat edge perfect for SOD-123 pad width
  - Good thermal mass
  - Efficient for repetitive soldering (90 diodes!)

**Temperature Setting**: 315-340°C (600-650°F) for lead-free solder

### Hand Soldering Technique for SOD-123:

```
Step-by-step process (per diode):
1. Pre-tin one pad (cathode side recommended)
2. Place diode with tweezers, align cathode band
3. Tack down pre-tinned side while holding diode
4. Solder other side properly
5. Re-flow first side if needed
6. Visual inspection (solder fillet on both sides)

Time per diode: ~20-30 seconds
Total for 90 diodes: ~30-45 minutes
```

### PCB Footprint Considerations:

**Pad Dimensions for SOD-123:**
- **Pad size**: 1.4mm × 1.0mm (per pad)
- **Pad spacing (center-to-center)**: 3.5mm
- **Thermal relief**: Not required (small thermal mass)
- **Silkscreen**: Cathode band marking on PCB

**Layout Tips:**
- Orient all diodes same direction for easier assembly
- Group by row/column for systematic soldering
- Clear silkscreen cathode indicator (line or arrow)
- Space between diode and switch for soldering access

---

## Updated Specification Sections

### Section 2.4 Key Matrix (Updated):

**Configuration**: 10 rows × 9 columns = 90 keys

**Implementation:**
- Matias Click switches (Alps-compatible)
- Alps-compatible keycaps ([source](https://matias.ca/switches/click/))
- ✅ **One SMD diode per switch (1N4148W, SOD-123 package)**
- Standard keyboard matrix configuration

**Diode Specifications:**
- **Package**: SOD-123 (3.5mm × 1.6mm × 1.1mm)
- **Part**: 1N4148W or equivalent
- **Hand-solderable**: Yes, with standard soldering iron
- **Recommended tip**: Hakko T18-C2 (2mm conical) or T18-D12 (1.2mm chisel)

### Section 5.1 Bill of Materials (Updated):

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **Diodes** | 1N4148W, SOD-123 | 90 | Hand-solderable SMD |

### Section 6.4 Critical Routing (Add subsection):

**Matrix Diodes (SOD-123):**
- Standard footprint: IPC-7351 SOD-123
- Pad size: 1.4mm × 1.0mm
- Pad spacing: 3.5mm (center-to-center)
- Orient all diodes consistently (cathode toward column or row)
- Silkscreen cathode band on all diode positions
- Clearance around pads for soldering iron access (~2mm minimum)

### Section 14.1 Outstanding Questions (Remove Question 3):

~~3. **Matrix Diodes**~~
   ~~- SMD vs through-hole diodes for switch matrix?~~
   ~~- If SMD: SOD-123 or SOD-323 package?~~
   ~~- If TH: standard DO-35 package?~~
   ~~- PCB routing considerations for each option~~

**Status**: ✅ **RESOLVED** - Using 1N4148W in SOD-123 package

---

## Assembly Notes for Documentation

### Tools Required for Diode Assembly:
- Hakko FX-888DX soldering iron with T18-C2 or T18-D12 tip
- Fine-point tweezers (ESD-safe preferred)
- 0.5-0.8mm solder wire (63/37 or SAC305 lead-free)
- Magnification (2.5x-5x helping hands or magnifying lamp)
- Flux pen (optional, but helpful)
- Isopropyl alcohol + brush for cleanup

### Assembly Order (Updated from Section 15):

```
PCB Assembly Order:
1. SMD diodes (90×) - Do these FIRST, before anything else
   - Reason: Flat PCB easier to work with
   - No components blocking access
   - Can use PCB edge for support
2. MCU (careful with orientation)
3. Passive components (resistors, capacitors)
4. USB connector
5. UPDI header
6. LEDs (careful with orientation)
7. Encoder
8. Switches (after all other components tested)
```

### Quality Control Checklist:

**After diode assembly (before proceeding):**
- [ ] Visual inspection: all 90 diodes soldered
- [ ] Orientation check: cathode bands aligned correctly
- [ ] Solder joint quality: shiny fillets on both pads
- [ ] No bridges between pads
- [ ] Continuity test: random sample of 10 diodes
- [ ] Clean flux residue with IPA

---

## Why SOD-123 Over Alternatives:

### SOD-123 vs SOD-323:
| Feature | SOD-123 | SOD-323 |
|---------|---------|---------|
| Size | 3.5mm × 1.6mm | 2.5mm × 1.3mm |
| Hand-solder | ✅ Easy | ⚠️ Challenging |
| Tweezers | Standard | Precision required |
| Reflow viable | Yes | Yes |
| Cost | Lower | Similar |
| **Verdict** | ✅ **Selected** | Too small for hand assembly |

### SOD-123 vs DO-35 (Through-hole):
| Feature | SOD-123 | DO-35 (TH) |
|---------|---------|------------|
| PCB space | Minimal | Requires holes + traces |
| Profile height | 1.1mm | 3-4mm (with leads) |
| Assembly time | 30-45 min (90 diodes) | 60-90 min |
| PCB routing | Easier (surface mount) | More complex |
| Rework | Easy (reflow) | Harder (desolder) |
| Cost | Similar | Similar |
| **Verdict** | ✅ **Selected** | Unnecessarily large |

---

## Updated Critical Issues List

### Critical (Block Progress) - Must Resolve Week 1:
1. ✅ **RESOLVED: Diode selection** → Using 1N4148W (SOD-123)
2. ⚠️ **PWM/matrix scan frequency conflict** - Still needs resolution
3. ⚠️ **Dev board availability** - Still needs verification

### Action Items for Week 1:
1. ✅ Order 100× 1N4148W (SOD-123) diodes (10% spare)
2. ✅ Verify Hakko T18-C2 or T18-D12 tip availability
3. ✅ Create SOD-123 footprint in KiCad (or use standard library)
4. ⚠️ Resolve PWM frequency specification
5. ⚠️ Confirm AVR64DU32 Nano dev board sourcing

---

**Recommendation**: With SOD-123 selected, schematic design can proceed. The footprint is standard and well-supported in KiCad libraries. Assembly will be straightforward with proper technique and the recommended soldering iron tip.



## Prompt:
12/25/2025, 10:26:04 PM

Note that I use leaded solder. The mcu flash can be configured to have a total of 3 sections: bootcode, appcode, and appdata. We’ve made it clear that the AmberClick FW Flashing Mode is running the bootcode section (first section). We’ve assumed so far that all of the Keyboard App would reside in the AppCode section, but should we make use of the ability to have an appdata section as well? The size of the appdata section is also selectable in a granular fashion (increments of 512 bytes).



## Response:
12/25/2025, 10:27:21 PM

# Analysis: AppData Section Usage

## Flash Memory Architecture - AVR64DU32

### Three-Section Configuration:

```
|------------------------| 0x0000
| BOOTCODE Section       |
| - HID Bootloader       |
| - Size: BOOTSIZE fuse  |
| - Protected from app   |
|------------------------| BOOTSIZE
| APPCODE Section        |
| - Keyboard firmware    |
| - Executable code      |
| - Size: 64KB - BOOTSIZE - APPDATASIZE |
|------------------------| 64KB - APPDATASIZE
| APPDATA Section        |
| - Non-volatile data    |
| - Size: APPDATASIZE    |
| - Write-protected      |
|------------------------| 0x10000 (64KB)
```

---

## Current Design (No AppData Section)

### What's Stored in Flash (AppCode):
```c
// All in APPCODE section currently:
const uint8_t keymap[LAYERS][ROWS][COLS] = {...};  // ~360 bytes
const macro_t macros[] = {...};                     // ~500 bytes
const encoder_action_t encoder_map[] = {...};      // ~50 bytes
const uint8_t led_brightness_default = 128;        // 1 byte
const char usb_device_name[] = "AmberClick90";     // ~20 bytes
// USB descriptors, etc.
```

**All data is read-only at compile time**

---

## Potential Use Cases for AppData Section

### 1. **User-Customizable Keymaps (Runtime Modification)**

**Concept**: Allow users to modify keymaps without reflashing entire firmware

**How it would work**:
```c
// APPCODE section (read-only defaults):
const uint8_t keymap_default[LAYERS][ROWS][COLS] PROGMEM = {...};

// APPDATA section (user-modifiable):
uint8_t keymap_user[LAYERS][ROWS][COLS] __attribute__((section(".appdata"))) = {...};

// At runtime:
if (appdata_valid_signature()) {
    use keymap_user;  // User customizations
} else {
    use keymap_default;  // Factory defaults
}
```

**Benefits**:
- ✅ VIA/VIAL-style keymap editing without full reflash
- ✅ User changes survive firmware updates (if AppData preserved)
- ✅ Factory reset available (reload defaults from AppCode)

**Drawbacks**:
- ❌ Requires bootloader or firmware to support AppData writes
- ❌ Additional complexity in firmware
- ❌ Wear leveling concerns for flash (though 10k erase cycles typical)

---

### 2. **User Preferences (Non-Volatile Settings)**

**What could be stored**:
```c
typedef struct {
    uint32_t magic_signature;     // Validity check
    uint8_t led_brightness_a;     // Current LED A brightness
    uint8_t led_brightness_b;     // Current LED B brightness
    uint8_t active_layer;         // Last active layer
    uint8_t encoder_sensitivity;  // Encoder speed
    uint8_t debounce_time;        // Custom debounce (ms)
    // ... more preferences
    uint16_t crc;                 // Data integrity
} user_prefs_t;
```

**Benefits**:
- ✅ Settings persist across power cycles
- ✅ No EEPROM needed (AVR64DU32 doesn't have EEPROM)
- ✅ User customization without firmware modification

**Drawbacks**:
- ❌ Flash write cycles limited (~10k)
- ❌ Current design already uses SRAM + defaults (simpler)
- ❌ Questionable value for a keyboard (how often do settings change?)

---

### 3. **Macro Storage (User-Definable Macros)**

**Concept**: Allow users to program custom macros without recompiling

**How it would work**:
```c
// APPCODE section (factory macros):
const macro_t macros_default[] PROGMEM = {...};

// APPDATA section (user macros):
macro_t macros_user[MAX_USER_MACROS] __attribute__((section(".appdata")));

// At runtime, combine both
```

**Benefits**:
- ✅ Dynamic macro programming (like QMK's dynamic macros)
- ✅ No recompilation required

**Drawbacks**:
- ❌ Complex to implement (macro recording UI)
- ❌ Limited use case (most users stick with predefined macros)
- ❌ Flash wear from frequent macro changes

---

### 4. **Calibration Data**

**What could be stored**:
```c
typedef struct {
    uint8_t led_gamma_table[256];    // Per-unit LED calibration
    int8_t encoder_offset;            // Encoder calibration
    uint16_t usb_vid;                 // Custom VID (if licensed)
    uint16_t usb_pid;                 // Custom PID
} calibration_data_t;
```

**Benefits**:
- ✅ Per-keyboard calibration without firmware variants
- ✅ Manufacturing/QC data storage

**Drawbacks**:
- ❌ Overkill for DIY project
- ❌ LEDs and encoders don't need calibration in this design

---

## Recommendation: **DO NOT USE AppData Section (Initially)**

### Rationale:

**1. YAGNI Principle (You Aren't Gonna Need It)**
- Current design is complete without AppData
- All data is compile-time constants
- No runtime customization required for basic operation

**2. Complexity vs Benefit**
- AppData requires:
  - Flash write routines in firmware or bootloader
  - Signature/CRC validation logic
  - Factory reset mechanism
  - Wear leveling considerations
- Benefit is minimal for a 90-key keyboard with 2 layers + macros

**3. Flash Write Limitations**
- Flash erase cycles: ~10,000 typical
- If user changes settings frequently: potential wear-out
- SRAM + compile-time defaults are simpler and unlimited write cycles

**4. No EEPROM Emulation Needed**
- AVR64DU32 has no EEPROM
- But we don't need persistent storage beyond power cycle
- Current design: defaults in Flash → loaded to SRAM at boot → modified at runtime → lost at power-off
- This is perfectly acceptable for a keyboard

**5. Future Expandability**
- Can add AppData section later if needed
- Firmware update preserves option to use AppData
- Easier to add than to remove

---

## Updated Memory Map (No AppData)

```
|------------------------| 0x0000
| BOOTCODE Section       |
| - HID Bootloader       |
| - Size: 4-6 KB         |
| - BOOTSIZE fuse        |
|------------------------| 0x1000 to 0x1800 (TBD)
| APPCODE Section        |
| - Keyboard firmware    |
| - Const data (keymaps) |
| - Size: 58-60 KB       |
|------------------------| 0x10000 (64KB)
| APPDATA Section        |
| - UNUSED (0 KB)        |
| - APPDATASIZE = 0x00   |
|------------------------| 0x10000
```

**Fuse Configuration**:
```
BOOTSIZE = 0x0A to 0x0C     // 5-6 KB bootloader
APPDATASIZE = 0x00          // 0 KB appdata (unused)
```

---

## Future Consideration: When AppData WOULD Be Valuable

### Scenario: VIA/VIAL Support (Advanced Feature)

If you later want to implement **VIA/VIAL-compatible dynamic keymap editing**:

**Then AppData becomes valuable**:
```c
// Reserve AppData section for user keymaps
APPDATASIZE = 0x02          // 1 KB appdata

Memory map:
|------------------------| 0x0000
| BOOTCODE (5 KB)        |
|------------------------| 0x1400
| APPCODE (58 KB)        |
|------------------------| 0xFC00
| APPDATA (1 KB)         |
| - User keymaps         |
| - User macros          |
|------------------------| 0x10000
```

**Benefits**:
- GUI-based keymap editor (like VIA Configurator)
- No firmware recompilation required
- User customization without technical knowledge

**Implementation**:
- Bootloader or firmware provides flash write capability
- VIA protocol for USB communication
- AppData stores user modifications
- AppCode stores factory defaults

---

## Updated Specification Sections

### Section 3.3 Memory Layout (Revised):

```
|------------------------| 0x0000
| BOOTCODE Section       |
| - HID Bootloader       |
| - Target: 4-6 KB       |
| - Fuse: BOOTSIZE       |
| - (512-byte increments)|
|------------------------| 0x1000 to 0x1800 (TBD)
| APPCODE Section        |
| - Keyboard firmware    |
| - Size: 58-60 KB       |
| - All keymaps/macros   |
| - USB descriptors      |
|------------------------| 0x10000 (64KB)
| APPDATA Section        |
| - UNUSED (0 KB)        |
| - Reserved for future  |
| - APPDATASIZE = 0x00   |
|------------------------| 0x10000
```

**Note**: AppData section not used in initial design. All configuration data stored as compile-time constants in AppCode section. AppData section can be enabled in future for VIA/VIAL support or user preferences if desired.

### Section 3.4 Data Storage Strategy (Add Note):

**Flash (Read-Only at Runtime):**
- Default keymaps (2 layers × 90 keys) - **stored in APPCODE section**
- Macro definitions - **stored in APPCODE section**
- USB descriptors - **stored in APPCODE section**
- Default LED brightness values - **stored in APPCODE section**
- Encoder action mappings - **stored in APPCODE section**

**SRAM (Runtime State):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness (loaded from flash defaults at boot)
- Macro execution state
- Current encoder position

**APPDATA Section:**
- **Not used in initial design**
- Reserved for future features (VIA/VIAL, user preferences)
- Fuse: APPDATASIZE = 0x00

**Rationale:**
- Keymaps are constants defined at compile-time
- Flash reads are single-cycle on AVR DU
- No performance penalty for flash-based lookup tables
- SRAM used only for runtime state (no persistence needed)
- AppData section adds complexity without clear benefit for basic keyboard operation

### Section 8.2 Fuse Settings (Update):

**Must Configure:**
```c
// Bootloader section
BOOTSIZE = 0x0A to 0x0C     // 5-6 KB, determined after bootloader compilation
                             // 512-byte increments

// AppData section  
APPDATASIZE = 0x00          // 0 KB (unused in initial design)
                             // Can be changed later for VIA/VIAL support

// Critical settings
UPDI: Enabled               // NEVER disable
BOD: TBD                    // Brown-out detection level
WDT: TBD                    // Watchdog configuration  
RSTPIN: Enabled             // Reset pin (encoder switch)
```

### Section 11 Future Enhancements (Add Item):

**Advanced Features:**
- VIA/VIAL support for runtime keymap editing
  - **Requires**: Enable APPDATA section (1-2 KB)
  - **Requires**: Flash write capability in firmware
  - **Benefits**: GUI-based keymap customization
- User preferences stored in APPDATA section
  - LED brightness persistence
  - Debounce timing customization
  - Encoder sensitivity settings
- RGB underglow (if desired)
- Audio output (piezo buzzer)
- Additional encoders
- OLED display

### Section 14.2 Outstanding Questions (Add Note to Question 7):

7. **Bootloader Configuration** ✅ **PARTIALLY RESOLVED**
   - ✅ **RESOLVED**: Fuse BOOTSIZE allows 512-byte increments (0 to 64KB)
   - ✅ **RESOLVED**: Set BOOTSIZE = (bootloader_bytes / 512) rounded up
   - ✅ **RESOLVED**: Fuse APPDATASIZE allows 512-byte increments
   - ✅ **DECISION**: APPDATASIZE = 0x00 (unused in initial design)
   - ❓ **REMAINING**: What is actual compiled bootloader size for our minimal HID implementation?
   - ❓ **REMAINING**: Reserve 4KB, 4.5KB, 5KB, 5.5KB, or 6KB? (Determine during bootloader development)

---

## Soldering Note Update

### Section: Assembly Notes (Add to existing):

**Soldering Iron Settings for Leaded Solder:**
- **Temperature**: 290-315°C (550-600°F) for 63/37 or 60/40 leaded solder
- **Tip**: Hakko T18-C2 (2mm conical) or T18-D12 (1.2mm chisel)
- **Solder Wire**: 0.5-0.8mm diameter, 63/37 or 60/40 tin-lead
- **Flux**: Rosin core in solder is usually sufficient; flux pen optional

**Temperature Comparison**:
- Lead-free (SAC305): 315-340°C
- Leaded (63/37): 290-315°C ← **Your setup**

**Benefits of Leaded Solder for Hand Assembly**:
- ✅ Lower melting point (easier on components)
- ✅ Better flow characteristics
- ✅ Shinier joints (easier visual inspection)
- ✅ More forgiving for beginners

---

## Summary

**Decision**: **Do NOT use AppData section initially**

**Fuse Configuration**:
```
BOOTSIZE = 0x0A (5 KB typical)
APPDATASIZE = 0x00 (0 KB - unused)
```

**Reasoning**:
1. ✅ Simpler design (YAGNI principle)
2. ✅ All data can be compile-time constants
3. ✅ No flash wear concerns
4. ✅ Easier to debug and maintain
5. ✅ Can add AppData later if VIA/VIAL support desired

**Future Path**:
- Implement basic keyboard first
- If users request runtime customization → add AppData support
- If VIA/VIAL integration desired → enable AppData section (1-2 KB)
- Firmware update can enable AppData without hardware changes

This keeps the initial design clean and focused while preserving future expandability.



## Prompt:
12/25/2025, 10:37:29 PM

we can write off the idea of a VIA/VIAL-compatible dynamic keymap editing functionality. users to compile and reflash when they want to change their keymap or macros.



## Response:
12/25/2025, 10:38:30 PM

# Updated: AppData Section Decision - FINAL

## Decision: **NO AppData Section - Ever**

### Rationale:

**Design Philosophy**:
- Users compile and reflash for keymap/macro changes
- No runtime customization needed
- Simpler, more maintainable codebase
- Traditional DIY keyboard approach (like original QMK workflow)

**Benefits of Compile-Time Configuration**:
1. ✅ **Simplicity**: No flash write routines needed
2. ✅ **Reliability**: No flash wear concerns
3. ✅ **Transparency**: All configuration in source code
4. ✅ **Version Control**: Keymaps tracked in git
5. ✅ **Reproducibility**: Exact firmware state known
6. ✅ **No Bricking Risk**: Can't corrupt configuration data
7. ✅ **Maximum AppCode Space**: Full 58-60KB available for firmware

**What Users Do**:
```bash
# User wants to change keymap:
1. Edit keymap.c
2. Recompile firmware
3. Enter bootloader (Fn + LeftCtrl + RightCtrl)
4. Flash new firmware via USB
5. Done (takes ~2 minutes total)
```

---

## Final Memory Map

```
|------------------------| 0x0000
| BOOTCODE Section       |
| - HID Bootloader       |
| - Size: 4-6 KB         |
| - BOOTSIZE fuse        |
|------------------------| 0x1400 (example: 5KB)
| APPCODE Section        |
| - Keyboard firmware    |
| - ALL keymaps          |
| - ALL macros           |
| - ALL configuration    |
| - Size: 59 KB          |
|------------------------| 0x10000 (64KB)

Total Flash Used: ~23-28 KB (bootloader + firmware)
Flash Remaining: ~36-41 KB free
```

**Fuse Configuration** (Final):
```c
BOOTSIZE = 0x0A          // 5 KB bootloader (adjust after compilation)
APPDATASIZE = 0x00       // 0 KB - NOT USED, NEVER WILL BE
```

---

## Updated Specification Sections

### Section 3.3 Memory Layout (Final Version):

```
|------------------------| 0x0000
| BOOTCODE Section       |
| - HID Bootloader       |
| - Target: 4-6 KB       |
| - Fuse: BOOTSIZE       |
| - (512-byte increments)|
|------------------------| 0x1000 to 0x1800 (TBD)
| APPCODE Section        |
| - Keyboard firmware    |
| - Size: 58-60 KB       |
| - All keymaps/macros   |
| - USB descriptors      |
| - All configuration    |
|------------------------| 0x10000 (64KB)
```

**Note**: AppData section is not used. All configuration is compile-time only. Users modify keymaps in source code and reflash firmware via USB bootloader.

### Section 3.4 Data Storage Strategy (Simplified):

**Flash (Compile-Time Constants in APPCODE):**
- Default keymaps (2 layers × 90 keys)
- Macro definitions
- USB descriptors
- Default LED brightness values
- Encoder action mappings
- **All configuration data**

**SRAM (Runtime State Only):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness (loaded from flash defaults at boot, reset on power cycle)
- Macro execution state
- Current encoder position

**No Persistent User Configuration**:
- Settings do not persist across power cycles
- LED brightness returns to default on boot
- All customization done via recompilation and reflashing
- Traditional DIY keyboard approach

**Rationale:**
- Maximum simplicity and reliability
- No flash wear concerns
- All configuration visible in source code
- Easy to version control and share configurations
- Users comfortable with this workflow (DIY keyboard community standard)

### Section 3.5 Firmware Features (Update Layer System):

#### Layer System
- 2 main layers with real keycodes (Layer 0, Layer 1)
- 1 system layer (Layer 2) for system functions:
  - LED brightness adjustment (runtime only, not persistent)
  - Bootloader entry (Fn + LeftCtrl + RightCtrl → firmware update mode)
  - Software reset key (firmware restart only, not bootloader entry)
  - System configuration (runtime adjustments, not saved)

**Customization Workflow**:
1. Edit `keymap.c` for layer definitions
2. Edit `macros.c` for macro definitions
3. Edit `config.h` for default brightness, debounce, etc.
4. Compile firmware
5. Enter bootloader via Fn + LeftCtrl + RightCtrl
6. Flash via USB HID bootloader
7. Keyboard restarts with new configuration

### Section 8.2 Fuse Settings (Final):

**Fuse Configuration**:
```c
// Memory sections
BOOTSIZE = 0x0A to 0x0C     // 5-6 KB, determined after bootloader compilation
APPDATASIZE = 0x00          // 0 KB - NOT USED

// Critical settings
UPDI: Enabled               // NEVER disable
BOD: TBD                    // Brown-out detection level
WDT: TBD                    // Watchdog configuration  
RSTPIN: Enabled             // Reset pin (encoder switch)
```

**Rationale**: Maximum APPCODE space for firmware. No runtime user data storage needed.

### Section 11 Future Enhancements (Simplified):

**Pin-Saving Techniques (if needed):**
- Share LEDs with matrix columns (LED active only when column driven)
- Encoder pins as matrix keys (careful decoding required)
- Note: Current design has sufficient GPIO, these are optional

**Advanced Features:**
- ~~VIA/VIAL support~~ **Removed - users reflash for changes**
- ~~User preferences in AppData~~ **Removed - compile-time only**
- RGB underglow (if desired)
- Audio output (piezo buzzer)
- Additional encoders
- OLED display

**Platform Scaling:**
- Firmware can be adapted for future AVR-DU variants with:
  - More than 64 KB Flash (more space for features)
  - More than 8 KB SRAM
  - More than 32 pins

### Section 12 References & Resources (Add):

**Firmware Customization**:
- QMK documentation (reference for keymap structure)
- Keyboard layout visualization tools (keyboard-layout-editor.com)
- Git for version control of personal keymaps
- Community keymap sharing (GitHub repositories)

### Section 14.2 Outstanding Questions (Remove AppData Items):

~~**AppData Section Usage**~~ **RESOLVED: Not used**

7. **Bootloader Configuration** ✅ **RESOLVED**
   - ✅ Fuse BOOTSIZE allows 512-byte increments (0 to 64KB)
   - ✅ Set BOOTSIZE = (bootloader_bytes / 512) rounded up
   - ✅ **APPDATASIZE = 0x00 (not used, users reflash for changes)**
   - ❓ What is actual compiled bootloader size? (Determine during development)
   - ❓ Reserve 4KB, 4.5KB, 5KB, 5.5KB, or 6KB? (Tune after bootloader complete)

---

## Firmware Source Structure (Example)

```
/AmberClick90-firmware
├── src/
│   ├── main.c                  # Entry point
│   ├── bootloader.c            # Bootloader code (separate section)
│   ├── keyboard.c              # Keyboard logic
│   ├── matrix.c                # Matrix scanning
│   ├── encoder.c               # Encoder handling
│   ├── keymap.c                # USER EDITS THIS - keymap definitions
│   ├── macros.c                # USER EDITS THIS - macro definitions
│   ├── config.h                # USER EDITS THIS - configuration
│   ├── hal_gpio.c              # Hardware abstraction
│   ├── hal_timer.c             # Timer/PWM
│   └── usb_descriptors.c       # USB configuration
├── Makefile                    # Build configuration
├── README.md                   # User guide, flashing instructions
└── keymaps/                    # Example keymaps
    ├── default/
    │   └── keymap.c
    ├── programmer/
    │   └── keymap.c
    └── gaming/
        └── keymap.c
```

**User Workflow**:
```bash
# Clone repository
git clone https://github.com/user/AmberClick90-firmware
cd AmberClick90-firmware

# Select or create keymap
cp keymaps/default/keymap.c src/keymap.c

# Edit keymap
nano src/keymap.c              # Or use Geany

# Edit configuration
nano src/config.h              # Adjust LED brightness, etc.

# Compile
make clean
make

# Flash (keyboard already in bootloader mode)
make flash

# Or manually:
# 1. Press Fn + LeftCtrl + RightCtrl on keyboard
# 2. Run: ./flash_tool AmberClick90.hex
```

---

## Benefits of This Approach

### For Users:
1. ✅ **Full Control**: All configuration visible in source
2. ✅ **Version Control**: Can track changes with git
3. ✅ **Sharing**: Easy to share complete configurations
4. ✅ **Reproducible**: Exact firmware state always known
5. ✅ **No Hidden State**: No mystery configuration in flash
6. ✅ **Community**: Can use standard QMK-style keymap format

### For Firmware:
1. ✅ **Simpler Code**: No flash write routines
2. ✅ **More Reliable**: No configuration corruption possible
3. ✅ **Faster Boot**: No AppData validation needed
4. ✅ **Easier Debug**: All state in SRAM, no persistent state
5. ✅ **More Space**: Extra 1-2 KB for features instead of AppData

### For Development:
1. ✅ **Less Testing**: No need to test flash write edge cases
2. ✅ **Faster Iteration**: Change keymap = recompile (fast)
3. ✅ **No Wear Concerns**: Flash writes only during firmware updates
4. ✅ **Cleaner Architecture**: Single source of truth (source code)

---

## Documentation Updates Needed

### User Manual (Section to Add):

**Customizing Your Keyboard**

AmberClick90 uses compile-time configuration. This means you edit the source code and reflash your keyboard when you want to make changes.

**Changing Your Keymap:**
1. Edit `src/keymap.c` with your preferred layout
2. Compile: `make`
3. Enter bootloader: Press Fn + LeftCtrl + RightCtrl on your keyboard
4. Flash: `make flash` or use your flashing tool
5. Keyboard automatically restarts with new keymap

**Changing Default LED Brightness:**
1. Edit `src/config.h`
2. Change `LED_BRIGHTNESS_DEFAULT` value (0-255)
3. Recompile and flash as above

**Adding Macros:**
1. Edit `src/macros.c`
2. Define your macro sequences
3. Reference macros in `src/keymap.c`
4. Recompile and flash

**Sharing Your Configuration:**
- Commit your `keymap.c`, `macros.c`, and `config.h` to git
- Share your repository with others
- Others can use your exact configuration

**Note**: Runtime changes (like Layer 2 LED brightness adjustments) are not saved when you power off. To make permanent changes, edit the source code.

---

## Summary

**Final Decision**: 
- **APPDATASIZE = 0x00** (permanent)
- **All configuration = compile-time**
- **Users reflash for changes**
- **No VIA/VIAL support**
- **Traditional DIY keyboard approach**

**Advantages**:
- Maximum simplicity and reliability
- Full control and transparency
- No hidden state or configuration corruption
- More flash space for features
- Standard DIY keyboard workflow

**This aligns with**:
- DIY keyboard community practices
- Original QMK workflow (before VIA)
- Unix philosophy: simple, composable tools
- Your project's minimalist aesthetic

The specification is now fully consistent with this compile-and-reflash philosophy.



## Prompt:
12/25/2025, 10:49:06 PM

what would be a good approach to host in the same github repo a sort of library of different macros (including encoder code such as the smart alt-tab) for users to explore and copy paste from in their own macro.c or encoder.c source files? of course, i don’t actually want to compile, link and include in appcode every possible macro example as that would be very inefficient if i don’t actually use them. i know there is the #define approach, but i find that ackward and rigid for this use case.



## Response:
12/25/2025, 10:50:36 PM

# Macro Library Architecture for AmberClick90

## Recommended Approach: Separate Examples Directory

### Repository Structure

```
/AmberClick90-firmware
├── src/                          # Active firmware source
│   ├── main.c
│   ├── keymap.c                  # User's active keymap
│   ├── macros.c                  # User's active macros
│   ├── encoder.c                 # User's active encoder code
│   ├── config.h
│   └── ...
├── lib/                          # Library of reusable components
│   ├── macros/                   # Macro library
│   │   ├── README.md             # Index and documentation
│   │   ├── basic/
│   │   │   ├── copy_paste.c      # Ctrl+C, Ctrl+V, etc.
│   │   │   ├── text_editing.c    # Line operations, word nav
│   │   │   └── navigation.c      # Home, End, PgUp, PgDn combos
│   │   ├── media/
│   │   │   ├── playback.c        # Play/pause, next, prev
│   │   │   └── volume.c          # Vol up/down, mute
│   │   ├── window_management/
│   │   │   ├── alt_tab.c         # Various Alt+Tab implementations
│   │   │   ├── virtual_desktops.c # Win+Ctrl+Left/Right
│   │   │   └── tiling.c          # Window snapping (Win+arrows)
│   │   ├── development/
│   │   │   ├── ide_shortcuts.c   # VSCode, IntelliJ shortcuts
│   │   │   ├── terminal.c        # Common terminal commands
│   │   │   └── git.c             # Git command sequences
│   │   ├── productivity/
│   │   │   ├── email.c           # Email signatures, templates
│   │   │   ├── zoom.c            # Zoom shortcuts
│   │   │   └── browser.c         # Tab management, bookmarks
│   │   └── custom/
│   │       └── template.c        # Empty template for custom macros
│   ├── encoders/                 # Encoder behavior library
│   │   ├── README.md
│   │   ├── basic_volume.c        # Simple volume control
│   │   ├── smart_alt_tab.c       # Smart Alt+Tab with timing
│   │   ├── scroll_wheel.c        # Mouse wheel emulation
│   │   ├── zoom_control.c        # Browser/app zoom
│   │   ├── brightness.c          # Screen brightness
│   │   ├── timeline.c            # Video/audio scrubbing
│   │   └── template.c            # Empty template
│   └── keymaps/                  # Example complete keymaps
│       ├── default/
│       │   └── keymap.c
│       ├── programmer/
│       │   ├── keymap.c
│       │   └── README.md         # Explains layout philosophy
│       ├── gaming/
│       │   └── keymap.c
│       └── vim_user/
│           └── keymap.c
├── docs/
│   ├── BUILDING.md
│   ├── FLASHING.md
│   ├── CUSTOMIZATION.md          # How to use the library
│   └── API.md                    # Macro/encoder API reference
├── Makefile
└── README.md
```

---

## Implementation Strategy

### 1. Self-Contained Library Files

Each library file is **complete and self-contained** with:
- Clear documentation
- Copy-paste ready code
- No dependencies on other library files

**Example: `lib/macros/window_management/alt_tab.c`**

```c
/*
 * Alt-Tab Macro Implementations
 * 
 * Copy the desired function(s) to your src/macros.c file.
 * These are complete, standalone implementations.
 */

#include <stdint.h>
#include "keyboard.h"  // Your keyboard API

/* ============================================================================
 * BASIC ALT-TAB
 * Simple Alt+Tab press (no hold logic)
 * ========================================================================== */
void macro_alt_tab_simple(void) {
    tap_key_with_mod(KC_TAB, MOD_ALT);
}

/* ============================================================================
 * SMART ALT-TAB (Hold Alt Across Multiple Tabs)
 * 
 * This version holds Alt down across multiple encoder rotations,
 * releasing it after a timeout.
 * 
 * USAGE:
 *   - Add this to your encoder handler
 *   - Call macro_alt_tab_smart(DIRECTION_CW) for clockwise
 *   - Call macro_alt_tab_smart(DIRECTION_CCW) for counter-clockwise
 *   - Call macro_alt_tab_timeout_check() in your main loop
 * 
 * REQUIREMENTS:
 *   - Timer/tick counter in your main loop
 * ========================================================================== */

#define ALT_TAB_TIMEOUT_MS 1000  // Release Alt after 1 second of inactivity

static uint32_t alt_tab_last_action = 0;
static bool alt_tab_active = false;

void macro_alt_tab_smart(encoder_direction_t direction) {
    uint32_t now = millis();  // Your timer function
    
    if (!alt_tab_active) {
        // First rotation: press and hold Alt
        register_key(KC_LALT);
        alt_tab_active = true;
    }
    
    // Tap Tab or Shift+Tab
    if (direction == DIRECTION_CW) {
        tap_key(KC_TAB);
    } else {
        tap_key_with_mod(KC_TAB, MOD_SHIFT);
    }
    
    alt_tab_last_action = now;
}

// Call this in your main loop
void macro_alt_tab_timeout_check(void) {
    if (alt_tab_active) {
        uint32_t now = millis();
        if ((now - alt_tab_last_action) > ALT_TAB_TIMEOUT_MS) {
            unregister_key(KC_LALT);
            alt_tab_active = false;
        }
    }
}

/* ============================================================================
 * ALT-TAB WITH MANUAL RELEASE
 * 
 * Hold Alt while encoder is being rotated, release when encoder stops
 * for a full detent period.
 * 
 * Simpler than timeout version, no timer required.
 * ========================================================================== */

static bool alt_held_manual = false;
static uint8_t detent_counter = 0;

void macro_alt_tab_manual(encoder_direction_t direction) {
    if (!alt_held_manual) {
        register_key(KC_LALT);
        alt_held_manual = true;
    }
    
    if (direction == DIRECTION_CW) {
        tap_key(KC_TAB);
    } else {
        tap_key_with_mod(KC_TAB, MOD_SHIFT);
    }
    
    detent_counter = 0;  // Reset counter
}

// Call this in your encoder update function (every scan)
void macro_alt_tab_manual_update(void) {
    if (alt_held_manual) {
        detent_counter++;
        if (detent_counter > 50) {  // ~50ms of no rotation
            unregister_key(KC_LALT);
            alt_held_manual = false;
        }
    }
}

/* ============================================================================
 * ALT-ESC (Alternative Window Switcher)
 * 
 * Some users prefer Alt+Esc for linear window switching
 * ========================================================================== */
void macro_alt_esc(encoder_direction_t direction) {
    if (direction == DIRECTION_CW) {
        tap_key_with_mod(KC_ESC, MOD_ALT);
    } else {
        tap_key_with_mod(KC_ESC, MOD_ALT | MOD_SHIFT);
    }
}
```

---

### 2. User's Active Files (Copy-Paste Workflow)

**User's `src/macros.c`** (starts empty or with template):

```c
/*
 * macros.c - User's active macro implementations
 * 
 * Copy functions from lib/macros/ and paste here.
 * Only the macros you use will be compiled.
 */

#include "keyboard.h"

/* ============================================================================
 * MY ACTIVE MACROS
 * Copied from library on 2025-12-25
 * ========================================================================== */

// From lib/macros/basic/copy_paste.c
void macro_copy(void) {
    tap_key_with_mod(KC_C, MOD_CTRL);
}

void macro_paste(void) {
    tap_key_with_mod(KC_V, MOD_CTRL);
}

// From lib/macros/window_management/alt_tab.c
// Using the "smart" version with timeout

#define ALT_TAB_TIMEOUT_MS 1000
static uint32_t alt_tab_last_action = 0;
static bool alt_tab_active = false;

void macro_alt_tab_smart(encoder_direction_t direction) {
    uint32_t now = millis();
    
    if (!alt_tab_active) {
        register_key(KC_LALT);
        alt_tab_active = true;
    }
    
    if (direction == DIRECTION_CW) {
        tap_key(KC_TAB);
    } else {
        tap_key_with_mod(KC_TAB, MOD_SHIFT);
    }
    
    alt_tab_last_action = now;
}

void macro_alt_tab_timeout_check(void) {
    if (alt_tab_active) {
        uint32_t now = millis();
        if ((now - alt_tab_last_action) > ALT_TAB_TIMEOUT_MS) {
            unregister_key(KC_LALT);
            alt_tab_active = false;
        }
    }
}

// From lib/macros/media/volume.c
void macro_volume_up(void) {
    tap_key(KC_AUDIO_VOL_UP);
}

void macro_volume_down(void) {
    tap_key(KC_AUDIO_VOL_DOWN);
}

/* ============================================================================
 * MY CUSTOM MACROS
 * ========================================================================== */

void macro_email_signature(void) {
    send_string("Best regards,\nYour Name\nyour.email@example.com");
}
```

---

### 3. Library Index and Documentation

**`lib/macros/README.md`**:

```markdown
# AmberClick90 Macro Library

This directory contains a collection of ready-to-use macros.
Copy the functions you want to your `src/macros.c` file.

## How to Use

1. Browse the categories below
2. Open the relevant `.c` file
3. Read the documentation in the comments
4. Copy the function(s) you want
5. Paste into your `src/macros.c`
6. Wire up to your keymap or encoder
7. Compile and flash

**Only the macros you copy will be compiled into your firmware.**

## Categories

### Basic Operations (`basic/`)
- `copy_paste.c` - Ctrl+C, Ctrl+V, Ctrl+X, Ctrl+A
- `text_editing.c` - Line delete, word navigation, selection
- `navigation.c` - Home/End combos, document navigation

### Media Control (`media/`)
- `playback.c` - Play/pause, next, previous track
- `volume.c` - Volume up/down, mute, fine control

### Window Management (`window_management/`)
- `alt_tab.c` - 4 different Alt+Tab implementations (⭐ popular)
- `virtual_desktops.c` - Desktop switching (Win/Mac)
- `tiling.c` - Window snapping and arrangement

### Development (`development/`)
- `ide_shortcuts.c` - VSCode, IntelliJ, Vim shortcuts
- `terminal.c` - Common terminal commands and navigation
- `git.c` - Git command sequences

### Productivity (`productivity/`)
- `email.c` - Email signatures and templates
- `zoom.c` - Zoom meeting shortcuts
- `browser.c` - Tab management, bookmark shortcuts

### Templates (`custom/`)
- `template.c` - Empty template for creating your own macros

## Macro API Reference

See `/docs/API.md` for the full keyboard API documentation.

### Basic Functions
```c
void tap_key(keycode_t key);
void tap_key_with_mod(keycode_t key, uint8_t mods);
void register_key(keycode_t key);
void unregister_key(keycode_t key);
void send_string(const char* str);
uint32_t millis(void);  // Milliseconds since boot
```

### Modifiers
```c
MOD_CTRL   // Left Ctrl
MOD_SHIFT  // Left Shift
MOD_ALT    // Left Alt
MOD_GUI    // Left Win/Cmd
```

## Popular Combinations

### Encoder: Smart Alt-Tab
Copy from `window_management/alt_tab.c` - "SMART ALT-TAB" section

### Key: Copy/Paste/Cut
Copy from `basic/copy_paste.c` - all three functions

### Key: IDE Line Operations
Copy from `development/ide_shortcuts.c` - duplicate line, delete line
```

---

### 4. Encoder Library Example

**`lib/encoders/smart_alt_tab.c`**:

```c
/*
 * Smart Alt-Tab Encoder Behavior
 * 
 * Holds Alt across multiple encoder rotations, releasing after timeout.
 * 
 * COPY THIS TO YOUR src/encoder.c
 */

#include "keyboard.h"

#define ALT_TAB_TIMEOUT_MS 800

static uint32_t last_rotation = 0;
static bool alt_held = false;

void encoder_alt_tab_update(encoder_direction_t direction) {
    uint32_t now = millis();
    
    // First rotation in a while - hold Alt
    if (!alt_held || (now - last_rotation) > ALT_TAB_TIMEOUT_MS) {
        if (!alt_held) {
            register_key(KC_LALT);
            alt_held = true;
        }
    }
    
    // Tap Tab (or Shift+Tab for reverse)
    if (direction == DIRECTION_CW) {
        tap_key(KC_TAB);
    } else {
        tap_key_with_mod(KC_TAB, MOD_SHIFT);
    }
    
    last_rotation = now;
}

// Call this in your main keyboard loop
void encoder_alt_tab_timeout_check(void) {
    if (alt_held) {
        uint32_t now = millis();
        if ((now - last_rotation) > ALT_TAB_TIMEOUT_MS) {
            unregister_key(KC_LALT);
            alt_held = false;
        }
    }
}

/* ============================================================================
 * USAGE IN YOUR ENCODER.C:
 * 
 * void encoder_update(encoder_direction_t direction) {
 *     if (current_layer == 0) {
 *         encoder_alt_tab_update(direction);
 *     } else if (current_layer == 1) {
 *         // Different behavior on layer 1
 *         if (direction == DIRECTION_CW) {
 *             macro_volume_up();
 *         } else {
 *             macro_volume_down();
 *         }
 *     }
 * }
 * 
 * void keyboard_task(void) {
 *     // ... your matrix scan, etc.
 *     encoder_alt_tab_timeout_check();  // Add this
 * }
 * ========================================================================== */
```

---

### 5. User Documentation

**`docs/CUSTOMIZATION.md`**:

```markdown
# Customizing Your AmberClick90

## Philosophy

AmberClick90 uses a **library approach** for macros and encoder behaviors.

- Browse the library in `lib/macros/` and `lib/encoders/`
- Copy the code you want to your active source files
- Only what you copy gets compiled
- No bloat, no unused code

## Workflow

### Adding a Macro

1. **Browse the library**:
   ```bash
   ls lib/macros/
   ```

2. **Find what you want**:
   ```bash
   cat lib/macros/window_management/alt_tab.c
   ```

3. **Copy to your macros.c**:
   - Open `src/macros.c` in your editor
   - Copy the function(s) you want
   - Paste at the bottom of your `src/macros.c`

4. **Wire it up in keymap.c**:
   ```c
   [LAYER_0] = {
       {KC_A, KC_B, MACRO(0), ...},  // Key calls macro 0
       ...
   }
   ```
   
   ```c
   void process_macro(uint8_t id) {
       switch (id) {
           case 0:
               macro_alt_tab_smart(DIRECTION_CW);
               break;
       }
   }
   ```

5. **Compile and flash**:
   ```bash
   make clean && make flash
   ```

### Adding Encoder Behavior

Same process, but copy to `src/encoder.c` instead.

### Creating Your Own

1. Start with `lib/macros/custom/template.c`
2. Copy template to your `src/macros.c`
3. Modify to your needs
4. (Optional) Contribute back to `lib/macros/custom/`!

## Examples

### Example 1: Basic Copy/Paste

```c
// In src/macros.c (copied from lib/macros/basic/copy_paste.c):
void macro_copy(void) {
    tap_key_with_mod(KC_C, MOD_CTRL);
}

void macro_paste(void) {
    tap_key_with_mod(KC_V, MOD_CTRL);
}

// In src/keymap.c:
[LAYER_1] = {
    {MACRO(0), MACRO(1), ...},  // Copy and Paste on Fn layer
    ...
}
```

### Example 2: Smart Alt-Tab on Encoder

```c
// In src/encoder.c (copied from lib/encoders/smart_alt_tab.c):
// [paste the whole smart alt-tab implementation]

// Wire up:
void encoder_update(encoder_direction_t dir) {
    encoder_alt_tab_update(dir);
}

// In main loop:
void keyboard_task(void) {
    matrix_scan();
    encoder_alt_tab_timeout_check();  // Don't forget this!
}
```

## Library Organization

```
lib/
├── macros/           # Macro snippets by category
│   ├── basic/
│   ├── media/
│   ├── window_management/  ← Alt-Tab lives here
│   ├── development/
│   ├── productivity/
│   └── custom/
└── encoders/         # Complete encoder behaviors
    ├── smart_alt_tab.c
    ├── basic_volume.c
    ├── scroll_wheel.c
    └── template.c
```

## Contributing Your Macros

Got a cool macro? Share it!

1. Add your macro to `lib/macros/custom/your_name.c`
2. Document it well (see existing examples)
3. Submit a pull request
4. Help others discover useful macros!
```

---

## Advantages of This Approach

### ✅ No Preprocessor Complexity
- No `#ifdef ENABLE_MACRO_ALT_TAB`
- No tangled conditional compilation
- No config file to maintain

### ✅ Zero Unused Code
- Only copied functions get compiled
- No dead code in your binary
- Maximum flash efficiency

### ✅ Easy to Browse
- Open any `.c` file and read it
- Self-contained examples
- Clear documentation in each file

### ✅ Easy to Customize
- Copy and modify freely
- No library version conflicts
- Your code, your responsibility

### ✅ Easy to Share
- "Check out `lib/macros/window_management/alt_tab.c`"
- Direct link to specific implementation
- Others can copy exactly what they want

### ✅ Discoverable
- Directory structure shows what's available
- README files guide users
- Search-friendly (grep for features)

### ✅ No Build System Complexity
- Makefile stays simple
- Just compiles `src/*.c`
- No library linking needed

---

## Alternative: Light Helper Script (Optional)

If you want to make copying even easier:

**`tools/add-macro.sh`**:

```bash
#!/bin/bash
# Helper script to copy macros to your active source

MACRO_FILE="$1"
DEST="src/macros.c"

if [ -z "$MACRO_FILE" ]; then
    echo "Usage: ./tools/add-macro.sh lib/macros/path/to/file.c"
    exit 1
fi

if [ ! -f "$MACRO_FILE" ]; then
    echo "Error: $MACRO_FILE not found"
    exit 1
fi

echo "Appending $MACRO_FILE to $DEST..."
echo "" >> "$DEST"
echo "/* Copied from $MACRO_FILE on $(date) */" >> "$DEST"
cat "$MACRO_FILE" >> "$DEST"
echo "" >> "$DEST"

echo "Done! Don't forget to wire it up in your keymap.c or encoder.c"
```

**Usage**:
```bash
./tools/add-macro.sh lib/macros/window_management/alt_tab.c
```

---

## Summary

**Recommended Approach**: **Separate Library Directory with Copy-Paste Workflow**

**Structure**:
```
/lib/macros/          # Library (not compiled)
/lib/encoders/        # Library (not compiled)
/src/macros.c         # User's active code (compiled)
/src/encoder.c        # User's active code (compiled)
```

**Workflow**:
1. User browses `lib/`
2. User copies desired function to `src/`
3. User wires up in `keymap.c` or `encoder.c`
4. Only copied code gets compiled

**Benefits**:
- ✅ Zero bloat (only used code compiled)
- ✅ Simple (no preprocessor magic)
- ✅ Flexible (modify freely after copying)
- ✅ Discoverable (clear directory structure)
- ✅ Shareable (easy to reference specific implementations)

This approach is clean, efficient, and aligns perfectly with your compile-and-reflash philosophy.



## Prompt:
12/25/2025, 11:01:57 PM

no need for the add-macro.sh script



## Response:
12/25/2025, 11:03:45 PM

# Final Macro Library Architecture - APPROVED

## Updated Specification Sections

### Section 3.2 Firmware Structure (Revised):

**Repository Organization:**
```
/AmberClick90-firmware
├── src/                          # Active firmware (gets compiled)
│   ├── main.c
│   ├── bootloader.c
│   ├── keyboard.c
│   ├── matrix.c
│   ├── keymap.c                  # User's active keymap
│   ├── macros.c                  # User's active macros
│   ├── encoder.c                 # User's active encoder code
│   ├── hal_gpio.c
│   ├── hal_timer.c
│   ├── config.h
│   └── usb_descriptors.c
├── lib/                          # Library (NOT compiled)
│   ├── macros/                   # Macro library
│   │   ├── README.md             # Index and usage guide
│   │   ├── basic/
│   │   │   ├── copy_paste.c
│   │   │   ├── text_editing.c
│   │   │   └── navigation.c
│   │   ├── media/
│   │   │   ├── playback.c
│   │   │   └── volume.c
│   │   ├── window_management/
│   │   │   ├── alt_tab.c         # Multiple implementations
│   │   │   ├── virtual_desktops.c
│   │   │   └── tiling.c
│   │   ├── development/
│   │   │   ├── ide_shortcuts.c
│   │   │   ├── terminal.c
│   │   │   └── git.c
│   │   ├── productivity/
│   │   │   ├── email.c
│   │   │   ├── zoom.c
│   │   │   └── browser.c
│   │   └── custom/
│   │       └── template.c
│   ├── encoders/                 # Encoder behavior library
│   │   ├── README.md
│   │   ├── basic_volume.c
│   │   ├── smart_alt_tab.c       # ⭐ Popular
│   │   ├── scroll_wheel.c
│   │   ├── zoom_control.c
│   │   ├── brightness.c
│   │   ├── timeline.c
│   │   └── template.c
│   └── keymaps/                  # Example complete keymaps
│       ├── default/
│       ├── programmer/
│       ├── gaming/
│       └── vim_user/
├── docs/
│   ├── BUILDING.md
│   ├── FLASHING.md
│   ├── CUSTOMIZATION.md          # How to use the library
│   └── API.md                    # Keyboard API reference
├── Makefile
└── README.md
```

**Build System:**
- Makefile only compiles files in `src/` directory
- `lib/` directory is ignored by build system
- Users manually copy desired code from `lib/` to `src/`
- Zero unused code in compiled firmware

---

### New Section 3.7: Macro Library System

**Philosophy:**
AmberClick90 provides a library of reusable macro and encoder implementations that users can browse and copy into their active firmware source files.

**Design Principles:**
1. **Copy-Paste Workflow**: Users manually copy code they want to use
2. **Self-Contained**: Each library file is complete and standalone
3. **Zero Bloat**: Only copied code gets compiled into firmware
4. **No Preprocessor Magic**: No `#ifdef` complexity or configuration
5. **Easy to Customize**: Users can modify copied code freely
6. **Community Contributions**: Users can share their macros back to the library

**Usage Workflow:**
```bash
# 1. Browse library
cat lib/macros/window_management/alt_tab.c

# 2. Copy desired function(s) to your macros.c
# (Manual copy-paste in editor)

# 3. Wire up in keymap.c or encoder.c
# (Add function calls)

# 4. Compile and flash
make clean && make flash
```

**Library Organization:**

**Macro Library (`lib/macros/`):**
- **basic/** - Copy, paste, text editing, navigation
- **media/** - Playback control, volume
- **window_management/** - Alt-Tab variants, virtual desktops, tiling
- **development/** - IDE shortcuts, terminal commands, git sequences
- **productivity/** - Email templates, video conferencing, browser shortcuts
- **custom/** - Template for user-created macros

**Encoder Library (`lib/encoders/`):**
- **basic_volume.c** - Simple volume up/down
- **smart_alt_tab.c** - Alt-Tab with hold logic and timeout
- **scroll_wheel.c** - Mouse wheel emulation
- **zoom_control.c** - Browser/app zoom
- **brightness.c** - Screen brightness control
- **timeline.c** - Video/audio timeline scrubbing
- **template.c** - Empty template for custom behaviors

**Each Library File Contains:**
- Complete, self-contained implementation
- Detailed usage documentation
- Multiple variant implementations (where applicable)
- Clear API requirements
- Copy-paste ready code

**Example Library File Structure:**
```c
/*
 * alt_tab.c - Alt-Tab Macro Implementations
 * 
 * Copy the desired function(s) to your src/macros.c or src/encoder.c
 */

/* ============================================================================
 * BASIC ALT-TAB
 * Simple Alt+Tab press
 * ========================================================================== */
void macro_alt_tab_simple(void) {
    tap_key_with_mod(KC_TAB, MOD_ALT);
}

/* ============================================================================
 * SMART ALT-TAB (Hold Alt Across Multiple Tabs)
 * 
 * USAGE: [detailed instructions]
 * REQUIREMENTS: [list requirements]
 * ========================================================================== */
[implementation...]
```

**Contributing to Library:**
Users can contribute their custom macros:
1. Add macro to `lib/macros/custom/[username].c`
2. Document thoroughly (follow existing format)
3. Submit pull request
4. Share with community

**Advantages:**
- ✅ No preprocessor complexity
- ✅ Zero unused code compiled
- ✅ Easy to browse and discover
- ✅ Simple to customize after copying
- ✅ No library version conflicts
- ✅ Straightforward build system
- ✅ Community-friendly sharing model

---

### Section 7.1 Initial Development Tasks (Add Item):

**3. Core Firmware**
   - Matrix scanning loop
   - Debouncing algorithm
   - Layer switching logic (2 main + 1 system layer)
   - Macro engine
   - **Macro library structure and templates**
     - Create `lib/` directory structure
     - Write initial macro examples (copy/paste, Alt-Tab, volume)
     - Write initial encoder examples (basic volume, smart Alt-Tab)
     - Document API in `docs/API.md`
     - Write `docs/CUSTOMIZATION.md` guide

---

### Section 12 References & Resources (Update):

**Firmware Customization:**
- QMK documentation (reference for keymap structure)
- Keyboard layout visualization tools (keyboard-layout-editor.com)
- Git for version control of personal keymaps
- **AmberClick90 Macro Library** (`lib/macros/` and `lib/encoders/`)
- **Community macro contributions** (GitHub pull requests welcome)

---

### Section 13 Typographic Logo Design (Add Note):

**Additional Branding:**
- Consider logo for `lib/` directory README files
- Consistent visual identity in library documentation
- Optional: ASCII art logo for terminal-friendly docs

---

### New Section 16: Community & Contribution

**Contributing Macros:**

AmberClick90 encourages users to share useful macros and encoder behaviors back to the community.

**How to Contribute:**

1. **Create Your Macro**
   - Write a well-documented macro in `lib/macros/custom/[your_name].c`
   - Follow the existing format (header comments, usage instructions)
   - Include multiple variants if applicable

2. **Test Thoroughly**
   - Ensure code compiles cleanly
   - Test on your keyboard
   - Verify documentation is accurate

3. **Submit Pull Request**
   - Fork the repository
   - Add your macro to `lib/macros/custom/`
   - Update `lib/macros/README.md` to list your contribution
   - Submit PR with clear description

4. **Share with Community**
   - Post on keyboard forums
   - Share on social media with #AmberClick90
   - Help others discover useful macros

**Contribution Guidelines:**
- ✅ Self-contained (no external dependencies)
- ✅ Well-documented (usage, requirements, examples)
- ✅ Multiple variants welcome (give users choices)
- ✅ Follow existing code style
- ✅ Test before submitting
- ✅ Clear, descriptive function names

**Popular Community Contributions:**
- Application-specific macro packs (Photoshop, Premiere, CAD software)
- Language-specific text editing macros
- Operating system-specific shortcuts
- Accessibility features
- Gaming profiles

**Library Maintenance:**
- Core maintainer reviews contributions
- Quality control for documentation and code style
- Periodic cleanup of outdated/broken macros
- Community voting on most useful macros (⭐ featured)

---

## Documentation to Create

### 1. `lib/macros/README.md` (Detailed)

```markdown
# AmberClick90 Macro Library

A curated collection of ready-to-use macros for your AmberClick90 keyboard.

## How to Use

**Copy-Paste Workflow:**
1. Browse the categories below
2. Open the relevant `.c` file and read documentation
3. Copy the function(s) you want
4. Paste into your `src/macros.c`
5. Wire up in `src/keymap.c` or `src/encoder.c`
6. Compile and flash: `make clean && make flash`

**Only the code you copy will be compiled into your firmware.**

## Quick Start Example

```bash
# Find a macro you want:
cat lib/macros/basic/copy_paste.c

# Copy the function to src/macros.c (use your editor)
# Then reference it in src/keymap.c
```

## Categories

### 📋 Basic Operations (`basic/`)
Essential keyboard operations every user needs.

- **`copy_paste.c`** - Ctrl+C, Ctrl+V, Ctrl+X, Ctrl+A, Ctrl+Z
- **`text_editing.c`** - Line operations, word navigation, selection
- **`navigation.c`** - Home/End combos, document navigation

### 🎵 Media Control (`media/`)
Control audio and video playback.

- **`playback.c`** - Play/pause, next, previous track
- **`volume.c`** - Volume up/down, mute, fine control

### 🪟 Window Management (`window_management/`)
Efficient window and workspace control.

- **`alt_tab.c`** ⭐ - 4 different Alt+Tab implementations
  - Basic Alt+Tab
  - Smart Alt+Tab with timeout (most popular)
  - Manual release version
  - Alt+Esc alternative
- **`virtual_desktops.c`** - Desktop switching (Windows/Mac/Linux)
- **`tiling.c`** - Window snapping and arrangement (Win+Arrow)

### 💻 Development (`development/`)
Shortcuts for programmers and developers.

- **`ide_shortcuts.c`** - VSCode, IntelliJ, Vim shortcuts
- **`terminal.c`** - Common terminal commands, navigation
- **`git.c`** - Git command sequences (commit, push, pull)

### 📊 Productivity (`productivity/`)
Boost efficiency in common applications.

- **`email.c`** - Email signatures and templates
- **`zoom.c`** - Zoom/Teams meeting shortcuts
- **`browser.c`** - Tab management, bookmark shortcuts

### ✨ Custom (`custom/`)
User-contributed and template macros.

- **`template.c`** - Empty template for creating your own
- **[Community contributions]** - Check for user-submitted macros

## API Reference

See `/docs/API.md` for complete keyboard API documentation.

### Common Functions

```c
// Single key tap
void tap_key(keycode_t key);

// Key with modifier(s)
void tap_key_with_mod(keycode_t key, uint8_t mods);

// Hold key down
void register_key(keycode_t key);

// Release key
void unregister_key(keycode_t key);

// Type a string
void send_string(const char* str);

// Get current time (for timeouts)
uint32_t millis(void);
```

### Modifiers

```c
MOD_CTRL   // Left Ctrl
MOD_SHIFT  // Left Shift  
MOD_ALT    // Left Alt
MOD_GUI    // Left Win/Cmd
```

Combine with `|`: `MOD_CTRL | MOD_SHIFT`

## Popular Combinations

**Want smart Alt-Tab on your encoder?**
→ Copy from `window_management/alt_tab.c` (SMART ALT-TAB section)

**Want copy/paste on your Fn layer?**
→ Copy from `basic/copy_paste.c` (all functions)

**Want IDE shortcuts?**
→ Copy from `development/ide_shortcuts.c` (pick your IDE)

## Contributing

Have a great macro? Share it with the community!

1. Add to `lib/macros/custom/your_name.c`
2. Document it thoroughly (follow existing format)
3. Submit a pull request
4. See contribution guidelines in main README.md

## Tips

- **Start simple**: Copy basic macros first, get familiar with the workflow
- **Customize freely**: After copying, modify to your exact needs
- **Comment your changes**: Future-you will thank you
- **Share discoveries**: Found a great combo? Share in discussions!

## Support

- Main documentation: `/docs/CUSTOMIZATION.md`
- API reference: `/docs/API.md`
- Issues: GitHub Issues
- Discussions: GitHub Discussions
```

---

### 2. `lib/encoders/README.md`

```markdown
# AmberClick90 Encoder Behavior Library

Complete encoder implementations for various use cases.

## How to Use

**Copy Entire Implementation:**
1. Open the encoder behavior file
2. Copy the complete implementation
3. Paste into your `src/encoder.c`
4. Wire up in your `encoder_update()` function
5. Don't forget timeout checks in main loop (if applicable)

## Available Behaviors

### 🔊 `basic_volume.c`
Simple volume control - clockwise = up, counter-clockwise = down.
**Complexity:** Beginner  
**Lines:** ~10

### 🪟 `smart_alt_tab.c` ⭐ Most Popular
Alt-Tab with intelligent hold logic. Holds Alt across multiple rotations, releases after timeout.
**Complexity:** Intermediate  
**Lines:** ~50  
**Requires:** Timer/tick counter

### 🖱️ `scroll_wheel.c`
Emulate mouse wheel scrolling.
**Complexity:** Beginner  
**Lines:** ~15

### 🔍 `zoom_control.c`
Browser/application zoom (Ctrl+Plus/Minus).
**Complexity:** Beginner  
**Lines:** ~12

### ☀️ `brightness.c`
Screen brightness control (function keys).
**Complexity:** Beginner  
**Lines:** ~12

### 🎬 `timeline.c`
Video/audio timeline scrubbing (Left/Right arrows).
**Complexity:** Intermediate  
**Lines:** ~30

### 📝 `template.c`
Empty template for custom encoder behaviors.
**Complexity:** Template  
**Lines:** ~20

## Quick Examples

### Basic Volume (Simplest)

```c
void encoder_update(encoder_direction_t direction) {
    if (direction == DIRECTION_CW) {
        tap_key(KC_AUDIO_VOL_UP);
    } else {
        tap_key(KC_AUDIO_VOL_DOWN);
    }
}
```

### Layer-Dependent Behavior

```c
void encoder_update(encoder_direction_t direction) {
    if (current_layer == 0) {
        // Layer 0: Volume
        encoder_volume_update(direction);
    } else if (current_layer == 1) {
        // Layer 1: Alt-Tab
        encoder_alt_tab_update(direction);
    }
}
```

## Contributing

Share your creative encoder behaviors!
See main README.md for contribution guidelines.
```

---

### 3. `docs/CUSTOMIZATION.md`

```markdown
# Customizing Your AmberClick90

## Overview

AmberClick90 uses a **copy-paste library** approach for customization:
- Browse reusable code in `lib/macros/` and `lib/encoders/`
- Copy what you want to your `src/` files
- Only copied code gets compiled
- No bloat, maximum flexibility

## Customization Workflow

### 1. Changing Your Keymap

Edit `src/keymap.c` to define key positions:

```c
const uint8_t keymap[LAYERS][ROWS][COLS] = {
    [LAYER_0] = {
        {KC_ESC,  KC_1,    KC_2,    ...},
        {KC_TAB,  KC_Q,    KC_W,    ...},
        ...
    },
    [LAYER_1] = {  // Fn layer
        {KC_GRV,  KC_F1,   KC_F2,   ...},
        {KC_CAPS, MACRO(0), MACRO(1), ...},
        ...
    }
};
```

### 2. Adding Macros from Library

**Step-by-step:**

```bash
# 1. Browse library
ls lib/macros/
cat lib/macros/window_management/alt_tab.c

# 2. Open your macros.c
nano src/macros.c  # or Geany

# 3. Copy desired function from library file
# 4. Paste at bottom of src/macros.c
# 5. Add comment noting source and date
```

**Example:**

```c
// In src/macros.c

/* Copied from lib/macros/basic/copy_paste.c on 2025-12-25 */
void macro_copy(void) {
    tap_key_with_mod(KC_C, MOD_CTRL);
}

void macro_paste(void) {
    tap_key_with_mod(KC_V, MOD_CTRL);
}
```

**Wire up in keymap:**

```c
// In src/keymap.c
void process_macro(uint8_t id) {
    switch (id) {
        case 0:
            macro_copy();
            break;
        case 1:
            macro_paste();
            break;
    }
}
```

### 3. Adding Encoder Behavior

Same process, copy to `src/encoder.c`:

```c
// In src/encoder.c

/* Copied from lib/encoders/smart_alt_tab.c on 2025-12-25 */

// [paste complete implementation]

void encoder_update(encoder_direction_t direction) {
    encoder_alt_tab_update(direction);
}

// In main keyboard loop:
void keyboard_task(void) {
    matrix_scan();
    encoder_alt_tab_timeout_check();  // Don't forget!
}
```

### 4. Compile and Flash

```bash
make clean
make
# Press Fn + LeftCtrl + RightCtrl on keyboard
make flash
```

## Configuration Options

Edit `src/config.h` for firmware-wide settings:

```c
// LED brightness (0-255)
#define LED_BRIGHTNESS_DEFAULT 128

// Matrix scan settings
#define DEBOUNCE_MS 5

// Encoder settings  
#define ENCODER_RESOLUTION 4  // Pulses per detent

// USB settings
#define USB_DEVICE_NAME "AmberClick90"
#define USB_MANUFACTURER "YourName"
```

## Library Organization

```
lib/
├── macros/
│   ├── basic/           # Copy, paste, text editing
│   ├── media/           # Playback, volume
│   ├── window_management/  # Alt-Tab, desktops, tiling
│   ├── development/     # IDE, terminal, git
│   ├── productivity/    # Email, zoom, browser
│   └── custom/          # Templates & community
└── encoders/
    ├── basic_volume.c
    ├── smart_alt_tab.c  ⭐
    ├── scroll_wheel.c
    └── template.c
```

## Tips & Best Practices

### Documentation
- Add comments when you copy from library
- Note the source file and date
- Explain any modifications you make

### Organization
- Group related macros together in your macros.c
- Use clear function names
- Keep your encoder.c simple and readable

### Version Control
- Commit your `src/` directory to git
- Track changes to your personal configuration
- Easy to revert or share your setup

### Sharing
- Share your complete `src/` folder
- Or share individual macros back to `lib/custom/`
- Help others discover useful combinations

## Common Recipes

### Recipe 1: Productivity Keyboard
```c
// Copy from library:
// - basic/copy_paste.c (all)
// - window_management/alt_tab.c (smart version)
// - productivity/email.c (signature)
// - encoders/basic_volume.c

// Result: Efficient office keyboard with smart Alt-Tab
```

### Recipe 2: Developer Keyboard
```c
// Copy from library:
// - development/ide_shortcuts.c (VSCode)
// - development/terminal.c (commands)
// - development/git.c (commit shortcuts)
// - encoders/scroll_wheel.c

// Result: Code-optimized layout
```

### Recipe 3: Media Workstation
```c
// Copy from library:
// - media/playback.c (all)
// - media/volume.c (fine control)
// - window_management/tiling.c
// - encoders/timeline.c

// Result: Video editing optimized
```

## Help & Support

- **Library index**: `lib/macros/README.md` and `lib/encoders/README.md`
- **API reference**: `docs/API.md`
- **Build guide**: `docs/BUILDING.md`
- **Flashing guide**: `docs/FLASHING.md`
- **GitHub Issues**: Bug reports and questions
- **GitHub Discussions**: Share configurations, ask for advice
```

---

## Summary

**Final Architecture**: Separate `lib/` directory with copy-paste workflow

**Key Points:**
1. ✅ `lib/` contains reusable, documented code (not compiled)
2. ✅ `src/` contains user's active code (gets compiled)
3. ✅ Users manually copy from `lib/` to `src/`
4. ✅ Zero bloat - only copied code is compiled
5. ✅ No build script complexity
6. ✅ Self-contained library files
7. ✅ Community contribution friendly
8. ❌ No `add-macro.sh` script needed

**Documentation Created:**
- `lib/macros/README.md` - Macro library index
- `lib/encoders/README.md` - Encoder library index  
- `docs/CUSTOMIZATION.md` - Complete user guide
- `docs/API.md` - Keyboard API reference (to be created)

This approach is clean, simple, and perfectly aligned with the compile-and-reflash philosophy while making macro discovery and sharing easy for the community.



## Prompt:
12/26/2025, 1:02:52 PM

prepare a new version of the ‘AmberClick90 (AVRDU.Enc.Mat) - Technical Specification’ that reflects our discussion and decisions so far in this chat. assume that i will capture the details of the macro and encoder library approach separately using the information provided at the bottom of this chat (no need to pull all of those details in the tech specs document as well as that would just duplicate things).



## Response:
12/26/2025, 1:06:00 PM

# AmberClick90 (AVRDU.Enc.Mat) - Technical Specification

## 1. Project Overview

**AmberClick90** is a custom 90-key USB keyboard featuring:

- **Physical Layout**: 90 keys arranged in a 10×9 matrix (physically 5 rows × 18 columns)
- **Switches**: Matias Click mechanical switches (Alps-compatible)
- **Encoder**: One ALPS EC11 rotary encoder with integrated push switch
- **Connectivity**: USB Full-Speed via USB Mini-B connector
- **Indicators**: 3 amber LEDs (1 power, 2 PWM-controlled indicators)
- **Aesthetics**: Minimalist black PCB with minimal grey 3D-printed case
- **Programming**: UPDI for initial/recovery flashing, USB HID bootloader for updates
- **Customization**: Compile-time configuration with macro/encoder library for code reuse

---

## 2. Hardware Specifications

### 2.1 Microcontroller: AVR64DU32

**Key Specifications:**
- 64 KB Flash memory
- 8 KB SRAM
- 24 MHz internal oscillator
- 32-pin package with 23 usable GPIO pins
- Native USB Full-Speed (no external PHY required)
- UPDI programming interface
- Internal USB regulator (3.3V)
- Operates at 5V input (from USB)

**Datasheet:** [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)

**Design Philosophy:**
- Firmware developed specifically for AVR64DU32 (best MCU in AVR-DU family currently)
- Moderate forward compatibility with future AVR-DU variants featuring:
  - More than 64 KB Flash
  - More than 8 KB SRAM
  - More than 32 pins
- Code structure allows future porting without current multi-variant complexity

**Advantages over ATmega32U4:**
- 2× flash capacity
- 3× SRAM capacity
- Better USB implementation
- No external crystal required
- UPDI instead of ISP programming
- Cleaner power design

### 2.2 Power Configuration

**Design**: USB Bus Powered (Datasheet Power Configuration 5b)

- VBUS → Polyfuse (500mA) → VDD (5V domain from USB host)
- Internal regulator provides VUSB (3.3V for USB core)

**Decoupling Requirements:**
- Bulk capacitor on VDD near USB connector (≤10µF per USB specification)
- Local decoupling capacitors (100nF, 0805) near all MCU VDD pins
- VUSB pin decoupled to ground only (no external load, 100nF, 0805)

**Power Protection:**
- Resettable polyfuse (500mA) between VBUS and VDD

### 2.3 GPIO Pin Allocation

**Total Usable GPIO: 23 pins**

| Function | Pin Assignment | Count |
|----------|----------------|-------|
| **Matrix Columns (9)** | PA0–PA3, PD1–PD5 | 9 |
| **Matrix Rows (10)** | PC3, PD0, PD6, PD7, PF0–PF5 | 10 |
| **Encoder A/B** | PA6, PA7 | 2 |
| **PWM Indicator LEDs** | PA4 (LED A), PA5 (LED B) | 2 |
| **Total** | | **23** |

**Pin Grouping Rationale:**
- Ports grouped to minimize register access during scanning
- Faster matrix scanning performance
- Cleaner PCB routing
- Reduced firmware complexity

**Reserved Pins (not counted as GPIO):**
- RESET pin (connected to encoder push switch)
- UPDI pin (programming interface)

### 2.4 Key Matrix

**Configuration**: 10 rows × 9 columns = 90 keys

**Implementation:**
- Matias Click switches (Alps-compatible mount)
- Alps-compatible keycaps ([available from Matias](https://matias.ca/switches/click/))
- One SMD diode per switch (1N4148W, SOD-123 package)
- Standard keyboard matrix configuration

**Switch Datasheet:** [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)

**Diode Specifications:**
- **Part Number**: 1N4148W (SOD-123 package)
- **Package Size**: 3.5mm × 1.6mm × 1.1mm
- **Hand-solderable**: Yes, with standard soldering iron
- **Pad dimensions**: 1.4mm × 1.0mm per pad
- **Pad spacing**: 3.5mm center-to-center

**Keycap Compatibility:**
- Matias switches use Alps mount (not Cherry MX)
- Alps-compatible keycaps required
- Verify keycap set availability before ordering switches

**Scanning Method:**
- Row-driven or column-driven (determine during firmware development)
- Debouncing in firmware (~5-20ms, tune during testing)
- Scan rate: 1-2 kHz typical

### 2.5 Rotary Encoder

**Model**: ALPS EC11 with detent clicks

**Datasheet:** [ALPS EC11 Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)

**Connections:**
- Encoder Phase A → PA6
- Encoder Phase B → PA7
- Integrated push switch → RESET pin (hardware reset only)

**Encoder Push Switch Behavior:**
- Connected directly to RESET pin
- Acts as hardware reset button
- Triggers firmware restart (not bootloader entry unless GPR flag set)
- Does NOT enter bootloader mode by itself

**Firmware Handling:**
- Quadrature decoding in main scan loop
- State machine or table-driven decoder
- Light debouncing (1-2ms) due to mechanical bounce
- Can implement "smart" behaviors (e.g., Alt-Tab with hold logic)

**Encoder Actions (Examples):**
- Clockwise: Volume up / Alt+Tab / Zoom in
- Counter-clockwise: Volume down / Alt+Shift+Tab / Zoom out
- Mappable to macros stored in flash
- Layer-dependent behavior supported

### 2.6 LED Configuration

#### Power LED (Non-PWM)
- **Type**: 3mm through-hole amber LED, diffused
- **Connection**: VDD (5V) with current-limiting resistor
- **Target Current**: 0.3-0.5mA (intentionally dim)
- **Resistor Selection**: ~6kΩ (test in dark environment to ensure non-annoying brightness)
- **Brightness**: Fixed, always-on, visible but subtle
- **Placement**: Resistor near LED (not MCU) for easier rework

#### Indicator LEDs (2× PWM-Controlled)
- **Type**: 3mm through-hole amber LEDs, diffused
- **Connections**: 
  - LED A → PA4 (TCA PWM capable)
  - LED B → PA5 (TCA PWM capable)
- **Target Current**: ~5mA per LED at 100% duty cycle
- **Forward Voltage**: ~2.0V (typical amber)
- **Resistor Value**: 270Ω (standard value, R = (3.3V - 2.0V) / 0.005A ≈ 260Ω)
- **Resistor Power**: ~7mW (1/8W or 1/4W suitable)
- **Placement**: Resistor near LED (not MCU)

**PWM Configuration:**
- **Timer**: TCA (Timer Counter A) - single timer drives both channels
- **Frequency**: 1-2 kHz (avoid aliasing with matrix scan)
- **Resolution**: 8-bit (0-255)
- **Default brightness**: Stored in Flash, loaded to SRAM at boot
- **Runtime brightness**: Stored in SRAM, adjustable via Layer 2
- **Brightness range**: 5-10% minimum (visible), 100% maximum (bright indoor use)
- **Persistence**: Runtime adjustments lost at power-off (returns to defaults)

**LED Advantages (3mm Diffused):**
- Smaller emitting area reduces glare
- Diffusion hides PWM artifacts
- Softer appearance in low light
- 60-100° viewing angle ideal for indicators

### 2.7 USB Interface

**Connector**: USB Mini-B, through-hole

**Design Requirements:**
- MCU placed as close to USB connector as possible (without compromising LED, switch, and encoder placement)
- Route D+/D- as short, matched-length differential pair (~90Ω impedance)
- Minimize stubs and avoid vias if possible
- Series resistors on D+ and D- lines (22-33Ω, 0805 SMD)
- Internal USB pull-up (verify MCU provides, or add external 1.5kΩ on D+)
- USB bulk capacitor (≤10µF total per USB spec, near connector)
- Optional: Low-capacitance USB ESD protection diode array
- Polyfuse (500mA) between VBUS and VDD

**USB Operation:**
- No external crystal oscillator required
- Internal 24MHz oscillator with USB clock recovery (SOF-based)
- USB Full-Speed compliant

### 2.8 Programming Interfaces

#### UPDI Header
- **Location**: PCB edge (top edge preferred for case design, or alternate edge for cleaner routing)
- **Pins**: 3 or 4 pins (consult AVR64DU32 datasheet for exact pinout)
- **Purpose**: 
  - Initial firmware programming
  - Recovery/unbrick capability
  - Development/debugging
- **Critical**: Never disable UPDI via fuses

#### USB Bootloader (HID)
- **Type**: HID bootloader (keyboard-class device)
- **Size**: 4-6 KB (BOOTSIZE fuse in 512-byte increments)
- **Entry Method**: Firmware key combination (Fn + LeftCtrl + RightCtrl)
- **Device Name**: "AmberClick90" (consider "AmberClick90 FW Update Mode" for bootloader)
- **Advantages**:
  - No drivers required on any OS
  - Same device class as keyboard
  - Works on locked-down systems
  - User-friendly firmware updates

---

## 3. Firmware Architecture

### 3.1 Development Toolchain

**Core Tools:**
- **Compiler**: avr-gcc
- **Editor**: Geany (or user preference)
- **UPDI Programmer**: SerialUPDI or compatible hardware programmer
- **USB Flashing**: Custom HID tool (research QMK Toolbox compatibility)
- **Development Board**: AVR64DU32 Nano dev board (for initial firmware development)

**Architecture**: Bare-metal + TinyUSB + minimal HAL

**No Arduino/QMK framework overhead - direct register access for performance and control**

### 3.2 Firmware Structure

**Repository Organization:**
```
/AmberClick90-firmware
├── src/                          # Active firmware (gets compiled)
│   ├── main.c
│   ├── bootloader.c
│   ├── keyboard.c
│   ├── matrix.c
│   ├── keymap.c                  # User's active keymap
│   ├── macros.c                  # User's active macros
│   ├── encoder.c                 # User's active encoder code
│   ├── hal_gpio.c
│   ├── hal_timer.c
│   ├── config.h
│   └── usb_descriptors.c
├── lib/                          # Library (NOT compiled, see separate doc)
│   ├── macros/                   # Copy-paste macro library
│   ├── encoders/                 # Copy-paste encoder library
│   └── keymaps/                  # Example complete keymaps
├── docs/
│   ├── BUILDING.md
│   ├── FLASHING.md
│   ├── CUSTOMIZATION.md
│   └── API.md
├── Makefile
└── README.md
```

**Build System:**
- Makefile compiles only files in `src/` directory
- `lib/` directory ignored by build system (reference library only)
- Users manually copy desired code from `lib/` to `src/`
- Zero unused code in compiled firmware

**Note**: See separate macro/encoder library documentation for details on the `lib/` directory structure and usage.

### 3.3 Memory Layout

#### Flash (64 KB total)

**Three-Section Architecture:**
```
|------------------------| 0x0000
| BOOTCODE Section       |
| - HID Bootloader       |
| - Size: 4-6 KB         |
| - BOOTSIZE fuse        |
| - (512-byte increments)|
|------------------------| 0x1000 to 0x1800 (TBD after development)
| APPCODE Section        |
| - Keyboard firmware    |
| - Size: 58-60 KB       |
| - All keymaps/macros   |
| - USB descriptors      |
| - All configuration    |
|------------------------| 0x10000 (64KB)
| APPDATA Section        |
| - NOT USED (0 KB)      |
| - APPDATASIZE = 0x00   |
|------------------------| 0x10000
```

**Memory Section Decisions:**
- **BOOTCODE**: 4-6 KB HID bootloader (size determined after implementation)
- **APPCODE**: Maximum space for keyboard firmware (58-60 KB)
- **APPDATA**: Not used - all configuration is compile-time only

**Fuse Configuration:**
```c
BOOTSIZE = 0x0A to 0x0C     // 5-6 KB (determined after bootloader compilation)
APPDATASIZE = 0x00          // 0 KB - NOT USED
```

**Rationale for No AppData:**
- Users compile and reflash for configuration changes
- No VIA/VIAL-style runtime editing
- Simpler, more reliable firmware
- No flash wear concerns
- All configuration visible in source code
- Traditional DIY keyboard approach

#### Flash Usage Breakdown (Application Only)

| Component | Size |
|-----------|------|
| Startup + HAL (GPIO, timer) | 1.5-2 KB |
| Matrix scanning (90 keys) | 1-1.5 KB |
| Debouncing logic | 0.5-1 KB |
| Layer engine (2 main + 1 system) | 0.5 KB |
| Macro engine (simple) | 0.5-1 KB |
| Keymap data (2×90 keys) | ~360 bytes |
| Keyboard logic glue | ~1 KB |
| **Non-USB Subtotal** | **6-8 KB** |
| TinyUSB core | 8-10 KB |
| HID class (keyboard, NKRO) | 3-4 KB |
| USB descriptors | ~0.5 KB |
| **USB Subtotal** | **12-14 KB** |
| **Total Application** | **18-22 KB** |
| **Free Space** | **36-42 KB** |

#### SRAM (8 KB total)

| Component | Size |
|-----------|------|
| Key matrix state (90 keys) | ~90 bytes |
| Debounce buffers | ~180 bytes |
| Macro buffers | 64-128 bytes |
| USB HID reports | 64-128 bytes |
| TinyUSB buffers | 1-1.5 KB |
| Stack | ~1 KB |
| **Total Used** | **2.5-3.5 KB** |
| **Free** | **4-5 KB** |

### 3.4 Data Storage Strategy

**Flash (Compile-Time Constants in APPCODE):**
- Default keymaps (2 main layers × 90 keys)
- System layer (Layer 2) definitions
- Macro definitions
- USB descriptors
- Default LED brightness values
- Encoder action mappings
- All configuration data

**SRAM (Runtime State Only):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness (loaded from flash defaults at boot)
- Macro execution state
- Current encoder position

**No Persistent User Configuration:**
- Settings do not persist across power cycles
- LED brightness returns to defaults on boot
- All customization done via source code modification and reflashing
- Runtime adjustments (Layer 2 brightness control) temporary only

**Rationale:**
- Keymaps are constants defined at compile-time
- Flash reads are single-cycle on AVR DU (no performance penalty)
- SRAM used only for runtime state (unlimited write cycles)
- Maximum simplicity and reliability
- No hidden state or configuration corruption risk
- Standard DIY keyboard workflow

### 3.5 Firmware Features

#### Layer System
- **Layer 0** (Base): Default alphanumeric layout
- **Layer 1** (Fn): Function keys, media controls, macros
- **Layer 2** (System): LED brightness adjustment, bootloader entry, software reset

**Layer 2 Functions:**
- LED brightness adjustment keys (runtime only, not persistent)
- Bootloader entry combo (Fn + LeftCtrl + RightCtrl → firmware update mode)
- Software reset key (firmware restart, does NOT enter bootloader)
- Optional system configuration (runtime adjustments)

**Customization Workflow:**
1. Edit `src/keymap.c` for layer definitions
2. Edit `src/macros.c` for macro definitions (copy from `lib/macros/`)
3. Edit `src/encoder.c` for encoder behavior (copy from `lib/encoders/`)
4. Edit `src/config.h` for defaults (brightness, debounce, etc.)
5. Compile firmware (`make`)
6. Enter bootloader (Fn + LeftCtrl + RightCtrl)
7. Flash via USB HID bootloader (`make flash`)
8. Keyboard restarts with new configuration

#### Macro Support
- Simple macros (key sequences, modifier combinations)
- Examples: Ctrl+C, Alt+Tab with hold logic, text snippets
- Stored in flash (APPCODE section)
- Execution state in SRAM
- **Library system**: Copy implementations from `lib/macros/` (see separate documentation)

#### NKRO (N-Key Rollover)
- Full NKRO HID descriptor (research required)
- All 90 keys can be pressed simultaneously
- USB HID report format to be defined during development

#### Encoder Features
- Quadrature decoding (state machine or table-driven)
- Macro dispatch on rotation
- Layer-dependent behavior
- **Smart behaviors**: Alt-Tab with hold logic, volume with acceleration, etc.
- **Library system**: Copy implementations from `lib/encoders/` (see separate documentation)

#### Debouncing
- **Matrix**: 5-20ms typical (tune during testing)
- **Encoder**: 1-2ms (lighter due to increased mechanical bounce)
- Software-based debouncing

### 3.6 USB Device Configuration

**USB Descriptors:**
- **Normal Mode**: "AmberClick90"
- **Bootloader Mode**: Research whether to use "AmberClick90 FW Update Mode"
- **Manufacturer**: [User's name/handle]
- **Version**: "AC90-r1" (or semantic versioning)

**HID Report Descriptor:**
- NKRO keyboard format (research TinyUSB implementation)
- Separate descriptors for bootloader vs application (research required)

---

## 4. Bootloader Implementation

### 4.1 Bootloader Type: HID (Keyboard-Class)

**Specifications:**
- Flash size: 4-6 KB (determined during development)
- SRAM: 300-600 bytes
- Enumeration: USB HID device
- Protocol: Custom (similar to QMK approach)
- No drivers required on host

**Advantages:**
- ✓ Driver-less operation on all OS (Windows, macOS, Linux)
- ✓ Same device class as keyboard
- ✓ Works on locked-down/corporate systems
- ✓ User-friendly firmware updates

### 4.2 Bootloader Entry Methods

#### Primary: Firmware Key Combination (Bootloader Entry)
- **Combo**: Fn + LeftCtrl + RightCtrl
- **Location**: Layer 0 or Layer 1 (via Fn key)
- **Detection**: During normal matrix scanning with debouncing
- **Process**: Firmware writes GPR magic value, triggers watchdog reset
- **Purpose**: Enter firmware update mode (USB HID bootloader)

#### Secondary: Software Reset Key (Firmware Restart Only)
- **Location**: Layer 2 (system layer)
- **Process**: Watchdog reset without setting GPR magic value
- **Purpose**: Quick firmware restart (does NOT enter bootloader)
- **Behavior**: Normal firmware startup after reset

#### Tertiary: Encoder Push Switch
- **Connection**: RESET pin (hardware)
- **Behavior**: Hardware reset, firmware restart
- **Note**: Does NOT enter bootloader (unless GPR flag happens to be set)

#### Recovery: UPDI Programming
- **Method**: Direct flash programming via UPDI interface
- **Purpose**: Unbrick corrupted firmware or bootloader
- **Requirement**: UPDI programmer hardware

### 4.3 Bootloader Request Flow

#### GPR Boot Flag Mechanism

**Magic Value Storage:**
- Use General Purpose Register r2 (or r3)
- Magic value: `0x42` (or other distinctive pattern)
- Survives watchdog reset (confirmed behavior)
- May survive RESET pin and direct software reset (research required)
- Does NOT survive power-on or brown-out reset

**Bootloader Entry (Fn + LeftCtrl + RightCtrl):**

```c
#define BOOT_MAGIC 0x42

// Step 1: Detect key combination in matrix scan
if (fn_layer_active &&
    key_is_pressed(KEY_LEFT_CTRL) &&
    key_is_pressed(KEY_RIGHT_CTRL)) {
    request_bootloader();
}

// Step 2: Set boot flag and reset
void request_bootloader(void) {
    asm volatile (
        "ldi r2, %0\n"
        :
        : "M" (BOOT_MAGIC)
    );
    wdt_enable(WDTO_15MS);
    while (1) {}  // Wait for watchdog reset
}

// Step 3: Early boot check (before C runtime init)
if (r2 == BOOT_MAGIC) {
    r2 = 0;  // Clear immediately to prevent false positives
    stay_in_bootloader();
} else {
    jump_to_application();
}
```

**Software Reset (Layer 2 Key):**

```c
// Does NOT set boot flag - firmware restarts normally
void software_reset(void) {
    // GPR r2 left unchanged (no magic value)
    wdt_enable(WDTO_15MS);
    while (1) {}
}
```

### 4.4 Reset Behavior Summary

| Reset Type | GPR Preserved? | Boot Flag | Result |
|------------|----------------|-----------|--------|
| Power-on | ❌ No | N/A | Normal firmware start |
| Brown-out | ❌ No | N/A | Normal firmware start |
| Watchdog (with flag) | ✅ Yes | Present | Enter bootloader |
| Watchdog (no flag) | ✅ Yes | Absent | Normal firmware start |
| RESET pin (encoder) | ❓ Research | N/A* | Normal firmware start* |
| Software Reset (L2) | ✅ Yes** | Absent | Normal firmware start |

\* Assumes no boot flag set; research GPR behavior on RESET pin  
\*\* Assumes watchdog-based reset

**Key Distinction:**
- **Bootloader Entry**: Sets boot flag → enters firmware update mode
- **Software Reset**: No boot flag → restarts firmware normally
- **Encoder Button**: Hardware reset → firmware restart (not bootloader)

### 4.5 Bootloader State Machine

```
Power On / Reset
    ↓
Bootloader Starts
    ↓
Check GPR r2 for BOOT_MAGIC
    ↓
   [Magic == 0x42?]
    ↓              ↓
   Yes            No
    ↓              ↓
Stay in         Jump to
Bootloader      Application
(USB HID)       (Keyboard)
```

**User-Visible Behavior:**

| Scenario | Boot Flag? | Result |
|----------|-----------|--------|
| Normal power-on | No | Keyboard starts immediately |
| Fn+LeftCtrl+RightCtrl | Yes | Enter bootloader (FW update mode) |
| Layer 2 software reset | No | Firmware restarts normally |
| Encoder button (RESET) | No* | Firmware restarts normally |
| UPDI programming | N/A | Direct flash write (bypass bootloader) |

\* Assumes GPR behavior on RESET pin (research required)

### 4.6 Bootloader Protection

**Fuse Configuration:**
- BOOTSIZE fuse reserves bootloader section (512-byte increments)
- Bootloader protected from application overwrites
- Application cannot write to bootloader section
- UPDI always enabled (NEVER disable)

**Jump Mechanism:**
```c
#define BOOTLOADER_ADDR 0x0000  // Adjust per BOOTSIZE fuse

void jump_to_application(void) {
    cli();  // Disable interrupts
    // Jump to application start
    ((void (*)(void))(BOOTLOADER_ADDR + bootloader_size))();
}
```

### 4.7 USB Enumeration Behavior

**From Host Perspective:**

1. **Normal Operation**: 
   - Device: "AmberClick90" keyboard
   - HID keyboard class

2. **Bootloader Entry (Fn+Ctrl+Ctrl)**: 
   - Keyboard disconnects
   - New device appears: "AmberClick90 [FW Update Mode]" (optional naming)
   - HID device class (keyboard-compatible)

3. **Firmware Update**: 
   - Host tool sends firmware via HID reports
   - Bootloader writes to flash

4. **Update Complete**: 
   - Device resets
   - Re-enumerates as "AmberClick90" keyboard

5. **Software Reset (Layer 2)**: 
   - Brief USB disconnect/reconnect
   - Same device (keyboard mode)

---

## 5. Component Selection

### 5.1 Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **MCU** | AVR64DU32-I/PT or similar | 1 | TQFP-32, use internal oscillator |
| **USB Connector** | USB Mini-B, through-hole | 1 | Full-Speed capable, board-mount |
| **UPDI Header** | 0.1" pitch, 3-4 pin | 1 | See datasheet for pinout |
| **USB Series Resistors** | 22-33Ω, 0805 | 2 | For D+, D- |
| **USB Bulk Cap** | 4.7-10µF, 0805 or larger | 1 | Near connector, X5R/X7R |
| **VDD Decoupling Caps** | 100nF, 0805 | 5-8 | One per VDD pin |
| **VUSB Decoupling Cap** | 100nF, 0805 | 1 | Near VUSB pin |
| **Polyfuse** | 500mA, 1812 or radial | 1 | Between VBUS and VDD |
| **ESD Protection** | Low-cap USB diode array | 1 | Optional (USBLC6-2SC6 or similar) |
| **Switches** | Matias Click | 90 | Alps-compatible, verify stock |
| **Keycaps** | Alps-compatible set | 90+ | See matias.ca for sources |
| **Diodes** | 1N4148W, SOD-123 | 100 | One per switch + spares |
| **Rotary Encoder** | ALPS EC11 with switch | 1 | With detent clicks |
| **Power LED** | 3mm amber, diffused, TH | 1 | Fixed brightness |
| **Indicator LEDs** | 3mm amber, diffused, TH | 2 | PWM-controlled |
| **Power LED Resistor** | ~6kΩ, 0805 or 1/4W TH | 1 | Test for desired brightness |
| **Indicator LED Resistors** | 270Ω, 0805 or 1/4W TH | 2 | Near LEDs |

**Additional Components:**
- Solder (63/37 or 60/40 leaded, 0.5-0.8mm diameter)
- Flux (rosin core or pen)
- Isopropyl alcohol (for cleanup)

### 5.2 Soldering Equipment

**Required for Assembly:**
- Soldering iron: Hakko FX-888DX (or equivalent temperature-controlled)
- Recommended tip: **Hakko T18-C2** (2mm conical) or **T18-D12** (1.2mm chisel)
- Temperature: 290-315°C (550-600°F) for leaded solder
- Fine-point tweezers (ESD-safe preferred)
- Magnification: 2.5x-5x (helping hands or magnifying lamp)
- Flush cutters (for LED/component leads)

**SOD-123 Diode Hand-Soldering:**
- Package size: 3.5mm × 1.6mm × 1.1mm (easily hand-solderable)
- Technique: Pre-tin one pad, tack diode, solder other side, re-flow first side
- Time: ~20-30 seconds per diode
- Total for 90 diodes: ~30-45 minutes

---

## 6. PCB Design Guidelines

### 6.1 Physical Layout

**Component Placement:**
- **USB connector**: Top edge, aligned with Fn key position
- **MCU**: As close to USB connector as possible (minimize D+/D- trace length)
- **Rotary encoder**: Top-left corner (accessible position)
- **Three LEDs**: Horizontal arrangement near top edge (visible, non-intrusive)
- **UPDI header**: Top edge (easier case design) or alternate edge (cleaner routing)
- **Switches**: Standard keyboard grid layout (uniform spacing)

**Silkscreen:**
- Project name: "AmberClick90"
- Trim designation: "(AVRDU.Enc.Mat)"
- Typographic logo (TBD - minimalist design)
- Component reference designators
- Cathode markings on all diodes (consistent orientation)
- Pin 1 indicators on MCU and connectors
- UPDI pin labels
- Monochrome aesthetic (white on black)

### 6.2 Design Specifications

- **PCB**: 2-layer, black solder mask, white silkscreen
- **Thickness**: Standard 1.6mm
- **Surface Finish**: ENIG or HASL (lead-free)
- **Case**: Minimal, grey 3D-printed (research filament options)
- **Aesthetic**: Minimalist, technical/instrument-like

### 6.3 Net Naming Convention

**Power:**
- VDD (5V)
- VUSB (3.3V internal)
- GND (single plane, no separate analog ground)

**Matrix:**
- ROW_0 through ROW_9
- COL_0 through COL_8

**Encoder:**
- ENC_A, ENC_B
- RESET (encoder switch and MCU pin)

**LEDs:**
- LED_PWR (power LED)
- LED_IND_A, LED_IND_B (PWM indicators)

**USB:**
- USB_DP, USB_DM
- VBUS

**Programming:**
- UPDI
- RESET

### 6.4 Critical Routing

**USB Differential Pair (Highest Priority):**
- Keep D+ and D- as short, matched-length traces
- Route as differential pair (~90Ω impedance)
- Minimize stubs (ideally zero)
- Avoid vias if possible (use if necessary, minimize stub length)
- Keep away from high-frequency signals (matrix scanning)
- Ground plane clearance appropriate for impedance

**Power Distribution:**
- Wide traces for VDD/GND (0.5-1mm minimum for VDD)
- Single ground plane on bottom layer (no splits)
- Star-ground topology



## Prompt:
12/27/2025, 10:45:52 PM

Continue



## Response:
12/27/2025, 10:48:49 PM

from USB connector
- Polyfuse placed immediately after USB connector
- Decoupling caps as close to IC pins as possible

**Matrix Routing:**
- Keep row/column traces away from USB differential pair
- Orient diodes consistently (all cathodes same direction)
- Silkscreen cathode band on all positions
- Allow ~2mm clearance around diode pads for soldering access
- Route to minimize trace crossings

**Diode Footprint (SOD-123):**
- IPC-7351 standard footprint
- Pad size: 1.4mm × 1.0mm
- Pad spacing: 3.5mm center-to-center
- Silkscreen cathode indicator (line or arrow)

**LED Placement:**
- Through-hole pads sized for 3mm LED leads
- Resistors near LEDs (not MCU)
- Polarity markings on silkscreen (flat edge, cathode)

---

## 7. Firmware Development Roadmap

### 7.1 Phase 1: Research & Planning (Weeks 1-2)

**Critical Research (Week 1):**
1. **DXCore Deep Dive**:
   - Fuse configurations (BOOTSIZE, APPDATASIZE, BOD, UPDI)
   - Startup code and linker scripts
   - UPDI flashing workflow
   - Clock configuration details

2. **TinyUSB + NKRO**:
   - NKRO HID descriptor format (90 keys)
   - TinyUSB integration with bare-metal AVR
   - Split descriptor strategy (bootloader vs application)
   - USB enumeration sequence

3. **Component Verification**:
   - AVR64DU32 Nano dev board availability and sourcing
   - Matias Click switch stock levels
   - Alps-compatible keycap availability
   - Lead times for long-lead items

4. **PWM Frequency Selection**:
   - Resolve PWM/matrix scan frequency conflict
   - Matrix: 1-2 kHz, PWM: Choose non-aliasing frequency (2-3 kHz or 500-800 Hz)
   - TCA timer configuration for PA4, PA5

**Parallel Activities (Weeks 1-2):**
- Logo design (minimalist, silkscreen-friendly)
- 3D case filament research (grey colors available)
- Test environment setup (Geany, avr-gcc, make)

### 7.2 Phase 2: Development Environment Setup (Weeks 2-3)

1. **Acquire Hardware**:
   - AVR64DU32 Nano dev board
   - UPDI programmer (SerialUPDI compatible)
   - Basic components for testing (LEDs, resistors)

2. **Toolchain Setup**:
   - Install avr-gcc (verify AVR DU support)
   - Configure Geany for AVR development
   - Test UPDI programming (blink test)
   - Research USB HID flashing tools

3. **Minimal HAL Development**:
   - GPIO abstraction (port-based access for AVR64DU32)
   - TCA PWM setup (test with LEDs on dev board)
   - Basic USB enumeration with TinyUSB
   - Watchdog configuration and reset behavior

4. **Library Structure**:
   - Create `lib/macros/` directory structure
   - Write initial macro examples (copy/paste, volume)
   - Write initial encoder examples (basic volume, smart Alt-Tab)
   - Document API in `docs/API.md`

### 7.3 Phase 3: Schematic Design (Weeks 3-4)

**Schematic Organization (KiCad):**

1. **Page 1 - Power**:
   - USB connector
   - Polyfuse
   - Bulk capacitor
   - VDD distribution
   - Symbol for VUSB

2. **Page 2 - MCU**:
   - AVR64DU32 with all pin assignments
   - VDD/VUSB decoupling capacitors
   - UPDI header
   - RESET connection (encoder switch)
   - Crystal oscillator (not used, but document)

3. **Page 3 - USB Interface**:
   - USB Mini-B connector pinout
   - D+/D- series resistors
   - Optional ESD protection
   - USB shield connection

4. **Page 4 - Key Matrix**:
   - Row/column connections to MCU
   - Diode placement (one per switch)
   - Switch footprint references

5. **Page 5 - Peripherals**:
   - Encoder connections (A, B, switch to RESET)
   - Three LEDs with resistors
   - PWM connections (PA4, PA5)

**Deliverable**: Complete schematic with DRC passed, ready for layout

### 7.4 Phase 4: Firmware Development on Dev Board (Weeks 4-6)

**Parallel with PCB Design/Fabrication:**

1. **Bootloader Implementation**:
   - GPR boot flag mechanism (verify watchdog reset preserves r2)
   - Test RESET pin behavior (research GPR preservation)
   - Minimal HID bootloader
   - Flash write routines
   - Application jump logic
   - USB enumeration in bootloader mode

2. **Core Keyboard Firmware**:
   - Matrix scanning (simulated on dev board with 3×3 test matrix)
   - Debouncing algorithm (tune timing)
   - Layer switching logic (3 layers)
   - NKRO HID descriptor and reporting

3. **Encoder Emulation**:
   - Quadrature decoder with two buttons on dev board
   - Test "smart" Alt-Tab behavior with timeout
   - Verify encoder state initialization

4. **LED Control**:
   - TCA PWM configuration (PA4, PA5)
   - 8-bit brightness control
   - Gamma correction (optional)
   - Verify no USB timing interference

### 7.5 Phase 5: PCB Design (Weeks 5-7)

**PCB Layout (KiCad):**

1. **Board Setup**:
   - Define board outline and mounting holes
   - Import switch positions (keyboard-layout-editor export or manual)
   - Set design rules (trace width, clearance, via size)

2. **Critical Component Placement**:
   - USB connector (top edge, aligned with Fn key)
   - MCU (as close to USB as possible)
   - Encoder (top-left corner)
   - LEDs (horizontal arrangement, top edge)
   - UPDI header (top or alternate edge)

3. **Critical Routing (in order)**:
   - USB differential pair (D+/D-, matched length, 90Ω, no vias)
   - Power traces (VDD, GND, wide traces, star ground)
   - Matrix rows and columns
   - Encoder signals
   - LED connections
   - Remaining signals

4. **Ground Plane and Finishing**:
   - Pour ground plane on bottom layer
   - Add thermal reliefs for through-hole pads
   - Silkscreen (logo, labels, component markings)
   - Design rule check (DRC)
   - Electrical rule check (ERC)

5. **Design Review**:
   - Verify all net connections
   - Check component footprints
   - Verify polarity markings
   - Review silkscreen clarity

**Deliverable**: PCB ready for fabrication (Gerber files)

### 7.6 Phase 6: Fabrication & Procurement (Weeks 6-10)

**PCB Fabrication (Weeks 6-8)**:
- Submit Gerber files to manufacturer (JLCPCB, PCBWay, etc.)
- Lead time: 2-4 weeks typical
- Order 5-10 boards (allow for mistakes)

**Component Ordering (Week 6)**:

*Order immediately after schematic finalized:*
- AVR64DU32 (verify stock, long lead item)
- Standard passives (caps, resistors - order extras)
- USB connector, UPDI header, polyfuse
- Encoder, LEDs

*Order after PCB layout validated:*
- Matias Click switches (90+ units, verify stock)
- Alps-compatible keycaps (verify availability)
- SMD diodes 1N4148W SOD-123 (100 units)

**During Fabrication**:
- Continue firmware development on dev board
- Prepare assembly workspace
- Verify soldering equipment ready
- Practice SOD-123 soldering technique

### 7.7 Phase 7: Assembly & Testing (Weeks 8-10)

**Assembly Order:**

1. **SMD Diodes (90×)** - FIRST, before anything else:
   - Flat PCB easier to work with
   - No components blocking access
   - Use PCB edge for support
   - Orient all diodes consistently
   - Visual inspection after completion

2. **MCU**:
   - Careful with TQFP-32 orientation (pin 1)
   - Flux and drag soldering technique
   - Inspect for bridges

3. **Passive Components**:
   - Resistors and capacitors
   - Near respective ICs

4. **USB Connector and UPDI Header**:
   - Through-hole, straightforward

5. **LEDs**:
   - Mind polarity (flat edge = cathode)
   - Through-hole, last SMD-related components

6. **Initial Testing** (before switches):
   - Visual inspection (no shorts, proper orientation)
   - Continuity testing (VDD, GND, critical nets)
   - Power-on test (measure VDD = 5V, VUSB = 3.3V)
   - UPDI programming test (flash blink test)
   - USB enumeration test (device appears)
   - Basic GPIO test (toggle pins, test LEDs)

7. **Encoder**:
   - After basic tests pass
   - Verify mechanical fit

8. **Switches** (only after firmware working):
   - Install all 90 switches
   - Test continuity of each position
   - Verify no shorts

**Full System Testing**:
- All 90 keys register correctly
- NKRO verification (10+ simultaneous keys)
- Layer switching (all 3 layers)
- Encoder rotation and detents
- Encoder push (reset)
- LED brightness control (PWM)
- Bootloader entry (Fn + LeftCtrl + RightCtrl)
- Software reset (Layer 2 key)
- Macro execution
- Firmware update via USB HID bootloader

### 7.8 Phase 8: Case Design & Final Assembly (Weeks 9-12)

1. **Case Design** (Week 9-10):
   - Wait for bare PCB to verify dimensions
   - Measure actual PCB dimensions (may differ slightly from design)
   - CAD model in Fusion 360 or OpenSCAD
   - Account for USB connector clearance
   - Account for LED visibility
   - Encoder knob design

2. **Test Print** (Week 10):
   - Print small section to verify tolerances
   - Test fit with PCB
   - Check switch clearances

3. **Final Print** (Week 11):
   - Select grey filament (based on research)
   - Full case print
   - Fit test with fully assembled PCB
   - Verify keycap clearances

4. **Final Assembly & Tuning** (Week 12):
   - Install keycaps
   - Fine-tune firmware (debounce, brightness, encoder sensitivity)
   - Burn-in testing (extended use)
   - Documentation (build notes, photos)

**Total Estimated Timeline**: 10-13 weeks from start to completion

---

## 8. Firmware Configuration Details

### 8.1 Clock Configuration

- **Primary Clock**: 24 MHz internal oscillator
- **USB Clock**: Derived via USB clock recovery (SOF-based)
- **Sleep Mode**: Research wake-on-interrupt for power efficiency (optional optimization)

### 8.2 Fuse Settings

**Must Configure (Critical):**
```c
// Memory sections
BOOTSIZE = 0x0A to 0x0C     // 5-6 KB bootloader (tune after compilation)
APPDATASIZE = 0x00          // 0 KB - NOT USED

// Protection and features
UPDI: Enabled               // NEVER DISABLE
BOD: [TBD]                  // Brown-out detection level (research)
WDT: [TBD]                  // Watchdog configuration (research)
RSTPIN: Enabled             // Reset pin (encoder switch)
```

**Research Required**:
- Optimal BOD level for 5V operation
- Watchdog fuse settings
- Any AVR64DU32-specific fuse considerations

### 8.3 Matrix Scanning Parameters

**Configuration (Tune During Testing):**
- **Scan rate**: 1-2 kHz typical
- **Debounce time**: 5-20ms (start at 10ms, adjust based on testing)
- **Pull configuration**: Research pull-up vs pull-down (consult DXCore)
- **Diode orientation**: Determine during schematic (COL2ROW vs ROW2COL)

**Port Optimization:**
- Rows grouped where possible → single register read
- Minimize port switching during scan
- Use bitmasks for efficient row/column access

### 8.4 PWM Configuration

**Timer: TCA (Timer Counter A)**
- Single timer drives both PWM channels (PA4, PA5)

**Parameters:**
- **Frequency**: 2-3 kHz or 500-800 Hz (avoid 1-2 kHz matrix scan aliasing)
- **Resolution**: 8-bit (0-255)
- **Prescaler**: Calculate based on 24 MHz clock
- **Waveform**: Fast PWM mode

**Brightness Control:**
```c
// Flash (compile-time defaults)
#define LED_BRIGHTNESS_DEFAULT 128  // 50% brightness

// SRAM (runtime, lost at power-off)
uint8_t led_brightness_a;
uint8_t led_brightness_b;

// At startup:
led_brightness_a = LED_BRIGHTNESS_DEFAULT;
led_brightness_b = LED_BRIGHTNESS_DEFAULT;

// Layer 2 keys adjust runtime values
```

**Optional Enhancement:**
- Gamma correction lookup table (8-bit to 8-bit) for perceptually linear brightness

### 8.5 Encoder Configuration

**Decoder Type:**
- State machine (preferred) or table-driven
- Handles mechanical bounce
- Detects forward/reverse rotation

**Sampling:**
- Poll during matrix scan loop (1-2 kHz)
- Or pin-change interrupt (optional optimization)

**Integration:**
- Encoder state updated in main loop
- Dispatch to macro functions based on direction and layer
- Timeout handling for "smart" behaviors (e.g., Alt-Tab)

**Example Action Mapping:**
```c
// Stored in Flash (APPCODE section)
const encoder_action_t encoder_map[] = {
    [LAYER_0] = { .cw = MACRO_VOL_UP,    .ccw = MACRO_VOL_DOWN },
    [LAYER_1] = { .cw = MACRO_ALT_TAB,   .ccw = MACRO_ALT_TAB_REV },
    [LAYER_2] = { .cw = MACRO_BRIGHT_UP, .ccw = MACRO_BRIGHT_DOWN }
};
```

---

## 9. Design Philosophy & Best Practices

### 9.1 Memory Management

**Flash Usage:**
- Store all constant data (keymaps, macros, descriptors) in APPCODE section
- No runtime writes (except via bootloader during firmware update)
- Maximize efficient use of 64 KB capacity
- Design allows scaling to future AVR-DU variants with >64 KB flash

**SRAM Usage:**
- Minimize static allocation
- Use only for runtime state (unlimited write cycles)
- Monitor stack usage (allocate ~1 KB safety margin)
- Design allows scaling to future AVR-DU variants with >8 KB SRAM

### 9.2 Code Quality

**Style:**
- Direct register access where performance-critical
- Minimal abstraction overhead
- Clean HAL for portability across AVR-DU family
- Well-commented for maintainability

**Portability:**
- Borrow proven concepts from QMK and DXCore
- Adapt specifically for AVR-DU architecture
- Document deviations from reference implementations
- Code structure allows future porting to larger AVR-DU variants

**Target Platform:**
- Firmware developed specifically for AVR64DU32
- Best MCU in AVR-DU family currently available
- No backward compatibility with smaller AVR-DU parts
- Forward compatibility via clean architecture, not #ifdef complexity

### 9.3 User Experience

**Boot Time:**
- Target: <100ms from power-on to functional keyboard
- Quick bootloader check with no perceptible delay

**Responsiveness:**
- Low-latency matrix scanning (1-2ms scan cycle)
- Immediate key feedback (no lag)
- Smooth encoder operation (no missed detents)

**Reliability:**
- Robust debouncing (no false triggers, no missed presses)
- Stable USB enumeration (works on first plug-in)
- Predictable behavior (no race conditions)

**Customization:**
- Clear source code organization
- Well-documented macro/encoder library
- Easy to modify and reflash
- Git-friendly workflow for version control

### 9.4 Development Principles

**YAGNI (You Aren't Gonna Need It):**
- No APPDATA section (not needed for target use case)
- No VIA/VIAL runtime editing (compile-time is sufficient)
- No complex build system (simple Makefile)

**KISS (Keep It Simple):**
- Copy-paste macro library (no preprocessor complexity)
- Direct register access (no HAL bloat)
- Single-purpose functions (easy to understand)

**Community-Oriented:**
- Open source firmware and documentation
- Shareable macro/encoder library
- Contribution-friendly structure

---

## 10. Testing & Validation

### 10.1 Hardware Testing Checklist

**Bare PCB (Before Switch Assembly):**
- [ ] Visual inspection (no shorts, solder bridges, proper orientation)
- [ ] Continuity testing (VDD to all VDD pins, GND continuity)
- [ ] Power-on test (measure VDD = 5V ±5%, VUSB = 3.3V ±5%)
- [ ] Polyfuse test (verify ~500mA trip point)
- [ ] UPDI programming (flash LED blink test)
- [ ] USB enumeration (device appears as "AmberClick90" or bootloader)
- [ ] Basic GPIO test (toggle test pins, verify with multimeter)
- [ ] LED test (all 3 LEDs light up, check polarity)
- [ ] PWM test (verify brightness control on indicators)

**After Switch Assembly:**
- [ ] All 90 switches register correctly (test with key tester firmware)
- [ ] No ghost keys (test problematic combinations)
- [ ] Encoder rotation (both directions, all detents)
- [ ] Encoder push (reset functionality, firmware restarts)

### 10.2 Firmware Testing Checklist

**Core Functionality:**
- [ ] Matrix scanning accuracy (every key detected)
- [ ] Debouncing effectiveness (no chattering, no missed presses)
- [ ] Layer switching (all 3 layers accessible and functional)
- [ ] NKRO verification (press 10+ keys simultaneously)
- [ ] Encoder decoding accuracy (all rotations detected, no phantom events)
- [ ] Encoder direction correct (CW/CCW match expected behavior)

**Special Functions:**
- [ ] LED brightness control (PWM working, visible brightness change)
- [ ] Layer 2 brightness adjustment (runtime control functional)
- [ ] Bootloader entry (Fn + LeftCtrl + RightCtrl → enters bootloader)
- [ ] Software reset (Layer 2 key → firmware restarts normally)
- [ ] Encoder reset (push switch → firmware restarts)
- [ ] Macro execution (test various macros from library)

**USB Functionality:**
- [ ] Firmware update via USB HID bootloader (successful flash)
- [ ] USB stability (no disconnects during normal use)
- [ ] Cross-platform compatibility (Windows, macOS, Linux)

### 10.3 Power Testing

**Current Draw Measurements:**
- [ ] Idle current (no keys pressed, default LED brightness)
- [ ] Active scanning (matrix scanning, encoder polling)
- [ ] All LEDs at 100% brightness
- [ ] All keys pressed simultaneously (NKRO stress test)
- [ ] Peak current during USB enumeration

**Targets:**
- Idle: <50 mA
- Active (typical use): 50-150 mA
- All LEDs + matrix: <300 mA
- Peak (everything on): <500 mA (must not trip polyfuse)

### 10.4 Long-Term Testing

- [ ] Burn-in test (24-hour continuous operation)
- [ ] Key endurance (repeated presses on high-use keys)
- [ ] Thermal stability (MCU temperature, LED heat)
- [ ] USB cable stress (multiple plug/unplug cycles)

---

## 11. Future Enhancements (Optional)

**Advanced Features (If Desired Later):**
- RGB underglow (addressable LEDs on PCB underside)
- Audio output (piezo buzzer for keypress feedback)
- Additional encoders (dual-encoder layout variant)
- OLED display (status, layer indicator, WPM counter)
- Per-key RGB backlighting (if Matias switches with clear housing available)

**Platform Scaling:**
- Port to future AVR-DU variants with more resources
- Larger keyboard layouts (100-key, full-size)
- Split keyboard variant (AmberClick90 Split)

**Software Enhancements:**
- USB MIDI support (use keyboard as MIDI controller)
- Mouse emulation (cursor control on Layer 2)
- Programmable key sequences (record macros on-the-fly, stored in SRAM)

**Not Planned (Explicitly Out of Scope):**
- VIA/VIAL runtime keymap editing (users reflash for changes)
- Wireless connectivity (Bluetooth module)
- Battery power (USB-powered only)
- APPDATA section usage (all configuration compile-time)

---

## 12. References & Resources

### 12.1 Datasheets

- [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)
- [ALPS EC11 Encoder Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)
- [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)
- [Matias Click Switch & Keycaps](https://matias.ca/switches/click/)
- [1N4148W Diode Datasheet](https://www.diodes.com/assets/Datasheets/ds30086.pdf)
- USB 2.0 Specification

### 12.2 Software & Tools

- **QMK Firmware** (reference implementation for keyboard concepts)
- **DXCore** (AVR DU support, startup code reference)
- **TinyUSB** (USB stack for embedded systems)
- **avr-gcc** (GCC compiler for AVR)
- **avrdude** (firmware flashing tool, verify AVR-DU support)
- **SerialUPDI** (UPDI programmer software)
- **KiCad** (PCB design)
- **Geany** (text editor for firmware development)

### 12.3 Standards & Specifications

- USB HID Usage Tables (keyboard scancodes)
- USB 2.0 Full-Speed Specification
- IPC-7351 (SMD footprint standards)
- IPC-2221 (PCB design standards)

### 12.4 Community Resources

- **Keyboard Layout Editor** (keyboard-layout-editor.com) - Layout visualization
- **Git** - Version control for personal configurations
- **GitHub** - Firmware repository, issue tracking, discussions
- **r/MechanicalKeyboards** - Community support and inspiration

### 12.5 Firmware Customization

See separate macro/encoder library documentation for:
- Copy-paste macro library (`lib/macros/`)
- Copy-paste encoder library (`lib/encoders/`)
- Example keymaps (`lib/keymaps/`)
- API reference (`docs/API.md`)
- Customization guide (`docs/CUSTOMIZATION.md`)

---

## 13. Typographic Logo Design

**Requirements:**
- Simple, silkscreen-friendly design
- Minimalist aesthetic matching project philosophy
- Technical/instrument-like appearance
- Monochrome (white silkscreen on black PCB)
- Incorporates "AmberClick90" or "AC90" branding
- Readable at PCB scale (~5-10mm height)

**Design Constraints:**
- Minimum line width: 0.15mm (typical silkscreen capability)
- No fine serifs or intricate details
- Clear letterforms at small sizes

**Design Status**: TBD (to be designed before PCB layout)

**Placement**: Top silkscreen, near USB connector or bottom corner

---

## 14. Outstanding Research Questions

### 14.1 Hardware Questions

1. **3D Printed Case**
   - What grey filament colors are available (PLA, PETG, ABS)?
   - Which grey shade best matches minimalist aesthetic?
   - Material choice for durability and printability

2. **Matias Switches & Keycaps**
   - ✅ **RESOLVED**: Alps-compatible keycaps available at [matias.ca](https://matias.ca/switches/click/)
   - Verify PCB footprint interchangeability between Matias Click and Matias Quiet Click
   - Current stock levels for 90+ switches
   - Lead times and pricing

3. **UPDI Programming**
   - UPDI programmer dongle options (SerialUPDI vs dedicated hardware)
   - Software tool compatibility (avrdude support for AVR64DU32?)
   - UPDI header pinout (3-pin vs 4-pin, verify from datasheet)

4. **USB Bootloader Tools**
   - QMK Toolbox compatibility with custom HID bootloader?
   - Need for custom host-side flashing tool?
   - Cross-platform flashing workflow (Windows/Mac/Linux)

### 14.2 Firmware Questions

5. **Reset Behavior**
   - ❓ Do GPR values survive RESET pin assertion? (datasheet suggests yes, needs testing)
   - ❓ Do GPR values survive direct software reset? (datasheet suggests yes, needs testing)
   - Test on dev board to confirm GPR preservation across all reset types

6. **USB Device Naming**
   - Should bootloader present different HID device name?
   - "AmberClick90" vs "AmberClick90 FW Update Mode"
   - User experience implications of name change during enumeration

7. **Clock & Power**
   - Optimal BOD (brown-out detection) level for 5V operation
   - Sleep mode implementation feasibility (power savings vs complexity)
   - Wake-on-interrupt approach for idle power reduction

8. **Matrix Scanning**
   - Row-driven vs column-driven performance comparison
   - Pull-up vs pull-down configuration (consult DXCore defaults)
   - Diode orientation (COL2ROW vs ROW2COL) - finalize during schematic

9. **NKRO Implementation**
   - HID descriptor format for full 90-key NKRO
   - USB report structure and size
   - Compatibility considerations across operating systems
   - TinyUSB NKRO example code

10. **USB HID Descriptor**
    - Setup requirements for custom HID descriptors in TinyUSB
    - Split descriptor approach (bootloader vs application)
    - Device string handling (manufacturer, product, serial)

11. **DXCore Research**
    - Pin mapping conventions for AVR64DU32
    - Recommended clock and fuse settings
    - Startup code and linker script templates
    - UPDI flashing methodology and tools

12. **PWM Timing**
    - Final PWM frequency selection (2-3 kHz or 500-800 Hz)
    - TCA timer configuration for dual-channel PWM on PA4, PA5
    - Gamma correction implementation (lookup table size, performance)
    - Verify no interference with USB timing

13. **Encoder Implementation**
    - Optimal quadrature decoder algorithm (state machine vs table)
    - "Smart" Alt-Tab timeout tuning (800ms? 1000ms?)
    - Encoder state initialization to prevent phantom rotations on boot

### 14.3 Power & Safety Questions

14. **Current Draw Analysis**
    - Worst-case current consumption calculation (spreadsheet)
    - Average current draw during typical use (estimate)
    - Peak current: all 90 keys + encoder + all LEDs at 100%
    - Polyfuse 500mA rating appropriate? (verify against calculations)

### 14.4 PCB & Assembly Questions

15. **Component Sourcing**
    - AVR64DU32 availability and pricing (Digikey, Mouser, LCSC)
    - Matias Click switches: stock levels, minimum order quantities
    - Alps-compatible keycaps: availability, cost, lead time
    - SOD-123 diodes: bulk pricing (100 units)

16. **Development Board**
    - AVR64DU32 Nano dev board: where to purchase?
    - Availability and lead time
    - Alternative: bare AVR64DU32 + breadboard + UPDI programmer?

---

## 15. Document Status

This specification represents the **consolidated design decisions** as of this revision and serves as:

- Complete reference for schematic and PCB design
- Firmware architecture guide
- Component selection reference
- Development roadmap and timeline
- Testing checklist
- Handoff document for future development
- Comprehensive list of research questions to resolve

**Key Design Decisions:**
- Firmware targets AVR64DU32 specifically (best AVR-DU currently available)
- Moderate forward compatibility with future larger AVR-DU variants (via architecture, not #ifdef)
- APPDATASIZE = 0x00 (all configuration compile-time, no VIA/VIAL)
- Layer 2 software reset restarts firmware (does NOT enter bootloader)
- Bootloader entry only via Fn+LeftCtrl+RightCtrl combination
- Encoder push switch is hardware reset only (not bootloader entry)
- Matias Click switches with Alps-compatible keycaps
- SOD-123 diodes (hand-solderable with Hakko T18-C2 or T18-D12 tip)
- Leaded solder (63/37, 290-315°C)
- Copy-paste macro/encoder library (no preprocessor complexity, zero bloat)
- Users reflash for all configuration changes (traditional DIY keyboard workflow)

**Next Steps:**
1. Resolve outstanding research questions (Section 14)
2. Finalize typographic logo design
3. Begin firmware development using AVR64DU32 Nano dev board
4. Create KiCad schematic (5 pages: power, MCU, USB, matrix, peripherals)
5. Complete PCB layout (2-layer, black/white, minimalist)
6. Develop and test bootloader on dev board
7. Order PCBs and components
8. Assemble and test first prototype
9. Design and print 3D case (grey filament)
10. Document build process and publish

---

*Document Version: 3.0*  
*Last Updated: December 27, 2025*  
*Project: AmberClick90 (AVRDU.Enc.Mat)*



## Prompt:
12/27/2025, 11:21:43 PM

The nano dev board is available quickly and affordable via mouser.ca. We will use MCC as the basis for the USB stack, trim it down to a pure HID device Full Speed (for the appcode) / vendor protocol device (libusb / winusb) (for bootloader, usb flashing mode), and borrow ideas from lufa: we will not use TinyUSB. avrdude is able to flash the mcu using the serialupdi programmer. avrusbboot is a tool (fw and software) we can borrow ideas from for the vendor protocol usb stack in the bootloader firmware side, and also on the software side (winusb + libusb may be required on windows). the footprints for the mathias click and mathias click silent is exactly the same, and so are the plate requirements. for clock setting i will just start with full speed as there are no constraints with power since the kb will be hard wired to a pc that’s plugged into an 110v outlet: we’ll however look at thermal implications. i now know how to determine the appcode and bootcode size for the fuse settings. the bootloader will not present itself as an hid, but rather just as a usb device (vendor protocol). we will not do nkro, but rather just 6kro. the updi connector on the pCb will be 3 pin. an updi pogrammer will be used (based on ch340 us to ttl chip), and that works with avrdude (serialupdi). ergonomic layout of the 90 switches is already done. scOpe out us midi, mOuse emulatin, and on the fLy macro recordng (gimmicks).



## Response:
12/27/2025, 11:24:45 PM

# AmberClick90 (AVRDU.Enc.Mat) - Technical Specification

## 1. Project Overview

**AmberClick90** is a custom 90-key USB keyboard featuring:

- **Physical Layout**: 90 keys in ergonomic layout (already designed)
- **Switches**: Matias Click or Matias Quiet Click (interchangeable footprints)
- **Encoder**: One ALPS EC11 rotary encoder with integrated push switch
- **Connectivity**: USB Full-Speed via USB Mini-B connector
- **Indicators**: 3 amber LEDs (1 power, 2 PWM-controlled indicators)
- **Aesthetics**: Minimalist black PCB with minimal grey 3D-printed case
- **Programming**: UPDI for initial/recovery flashing, USB vendor protocol bootloader for updates
- **Customization**: Compile-time configuration with macro/encoder library for code reuse

---

## 2. Hardware Specifications

### 2.1 Microcontroller: AVR64DU32

**Key Specifications:**
- 64 KB Flash memory
- 8 KB SRAM
- 24 MHz internal oscillator
- 32-pin package with 23 usable GPIO pins
- Native USB Full-Speed (no external PHY required)
- UPDI programming interface
- Internal USB regulator (3.3V)
- Operates at 5V input (from USB)

**Datasheet:** [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)

**Design Philosophy:**
- Firmware developed specifically for AVR64DU32 (best MCU in AVR-DU family currently)
- Moderate forward compatibility with future AVR-DU variants featuring:
  - More than 64 KB Flash
  - More than 8 KB SRAM
  - More than 32 pins
- Code structure allows future porting without current multi-variant complexity

**Advantages over ATmega32U4:**
- 2× flash capacity
- 3× SRAM capacity
- Better USB implementation
- No external crystal required
- UPDI instead of ISP programming
- Cleaner power design

**Development Hardware:**
- **AVR64DU32 Nano dev board** (available via mouser.ca, quick delivery, affordable)
- Used for initial firmware development and testing

### 2.2 Power Configuration

**Design**: USB Bus Powered (Datasheet Power Configuration 5b)

- VBUS → Polyfuse (500mA) → VDD (5V domain from USB host)
- Internal regulator provides VUSB (3.3V for USB core)

**Power Budget:**
- Hardwired to PC (plugged into 110V outlet)
- No power consumption constraints
- Full-speed operation (24 MHz) - no need for low-power modes
- Monitor thermal implications of full-speed continuous operation

**Decoupling Requirements:**
- Bulk capacitor on VDD near USB connector (≤10µF per USB specification)
- Local decoupling capacitors (100nF, 0805) near all MCU VDD pins
- VUSB pin decoupled to ground only (no external load, 100nF, 0805)

**Power Protection:**
- Resettable polyfuse (500mA) between VBUS and VDD

### 2.3 GPIO Pin Allocation

**Total Usable GPIO: 23 pins**

| Function | Pin Assignment | Count |
|----------|----------------|-------|
| **Matrix Columns (9)** | PA0–PA3, PD1–PD5 | 9 |
| **Matrix Rows (10)** | PC3, PD0, PD6, PD7, PF0–PF5 | 10 |
| **Encoder A/B** | PA6, PA7 | 2 |
| **PWM Indicator LEDs** | PA4 (LED A), PA5 (LED B) | 2 |
| **Total** | | **23** |

**Pin Grouping Rationale:**
- Ports grouped to minimize register access during scanning
- Faster matrix scanning performance
- Cleaner PCB routing
- Reduced firmware complexity

**Reserved Pins (not counted as GPIO):**
- RESET pin (connected to encoder push switch)
- UPDI pin (programming interface)

### 2.4 Key Matrix

**Configuration**: 10 rows × 9 columns = 90 keys

**Implementation:**
- Matias Click OR Matias Quiet Click switches (user choice, identical footprints)
- Alps-compatible keycaps ([available from Matias](https://matias.ca/switches/click/))
- One SMD diode per switch (1N4148W, SOD-123 package)
- Standard keyboard matrix configuration
- Ergonomic layout already designed

**Switch Datasheet:** [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)

**Switch Compatibility:**
- ✅ **Matias Click and Matias Quiet Click have identical footprints**
- ✅ **Plate requirements are identical between Click and Quiet variants**
- Users can choose switch type without PCB/plate changes

**Diode Specifications:**
- **Part Number**: 1N4148W (SOD-123 package)
- **Package Size**: 3.5mm × 1.6mm × 1.1mm
- **Hand-solderable**: Yes, with standard soldering iron
- **Pad dimensions**: 1.4mm × 1.0mm per pad
- **Pad spacing**: 3.5mm center-to-center

**Keycap Compatibility:**
- Matias switches use Alps mount (not Cherry MX)
- Alps-compatible keycaps required
- Available from Matias and other suppliers

**Scanning Method:**
- Row-driven or column-driven (determine during firmware development)
- Debouncing in firmware (~5-20ms, tune during testing)
- Scan rate: 1-2 kHz typical

### 2.5 Rotary Encoder

**Model**: ALPS EC11 with detent clicks

**Datasheet:** [ALPS EC11 Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)

**Connections:**
- Encoder Phase A → PA6
- Encoder Phase B → PA7
- Integrated push switch → RESET pin (hardware reset only)

**Encoder Push Switch Behavior:**
- Connected directly to RESET pin
- Acts as hardware reset button
- Triggers firmware restart (not bootloader entry unless GPR flag set)
- Does NOT enter bootloader mode by itself

**Firmware Handling:**
- Quadrature decoding in main scan loop
- State machine or table-driven decoder
- Light debouncing (1-2ms) due to mechanical bounce
- Can implement "smart" behaviors (e.g., Alt-Tab with hold logic)

**Encoder Actions (Examples):**
- Clockwise: Volume up / Alt+Tab / Zoom in
- Counter-clockwise: Volume down / Alt+Shift+Tab / Zoom out
- Mappable to macros stored in flash
- Layer-dependent behavior supported

### 2.6 LED Configuration

#### Power LED (Non-PWM)
- **Type**: 3mm through-hole amber LED, diffused
- **Connection**: VDD (5V) with current-limiting resistor
- **Target Current**: 0.3-0.5mA (intentionally dim)
- **Resistor Selection**: ~6kΩ (test in dark environment to ensure non-annoying brightness)
- **Brightness**: Fixed, always-on, visible but subtle
- **Placement**: Resistor near LED (not MCU) for easier rework

#### Indicator LEDs (2× PWM-Controlled)
- **Type**: 3mm through-hole amber LEDs, diffused
- **Connections**: 
  - LED A → PA4 (TCA PWM capable)
  - LED B → PA5 (TCA PWM capable)
- **Target Current**: ~5mA per LED at 100% duty cycle
- **Forward Voltage**: ~2.0V (typical amber)
- **Resistor Value**: 270Ω (standard value, R = (3.3V - 2.0V) / 0.005A ≈ 260Ω)
- **Resistor Power**: ~7mW (1/8W or 1/4W suitable)
- **Placement**: Resistor near LED (not MCU)

**PWM Configuration:**
- **Timer**: TCA (Timer Counter A) - single timer drives both channels
- **Frequency**: 1-2 kHz (avoid aliasing with matrix scan)
- **Resolution**: 8-bit (0-255)
- **Default brightness**: Stored in Flash, loaded to SRAM at boot
- **Runtime brightness**: Stored in SRAM, adjustable via Layer 2
- **Brightness range**: 5-10% minimum (visible), 100% maximum (bright indoor use)
- **Persistence**: Runtime adjustments lost at power-off (returns to defaults)

**LED Advantages (3mm Diffused):**
- Smaller emitting area reduces glare
- Diffusion hides PWM artifacts
- Softer appearance in low light
- 60-100° viewing angle ideal for indicators

### 2.7 USB Interface

**Connector**: USB Mini-B, through-hole

**Design Requirements:**
- MCU placed as close to USB connector as possible (without compromising LED, switch, and encoder placement)
- Route D+/D- as short, matched-length differential pair (~90Ω impedance)
- Minimize stubs and avoid vias if possible
- Series resistors on D+ and D- lines (22-33Ω, 0805 SMD)
- Internal USB pull-up (verify MCU provides, or add external 1.5kΩ on D+)
- USB bulk capacitor (≤10µF total per USB spec, near connector)
- Optional: Low-capacitance USB ESD protection diode array
- Polyfuse (500mA) between VBUS and VDD

**USB Operation:**
- No external crystal oscillator required
- Internal 24MHz oscillator with USB clock recovery (SOF-based)
- USB Full-Speed compliant
- Full-speed operation (no low-power modes needed)

### 2.8 Programming Interfaces

#### UPDI Header
- **Location**: PCB edge (top edge preferred for case design, or alternate edge for cleaner routing)
- **Pins**: 3-pin header (GND, UPDI, VDD or similar - verify from datasheet)
- **Purpose**: 
  - Initial firmware programming
  - Recovery/unbrick capability
  - Development/debugging
- **Critical**: Never disable UPDI via fuses

**UPDI Programmer:**
- Based on CH340 USB-to-TTL chip
- SerialUPDI protocol
- Works with avrdude
- Affordable and readily available

**Programming Software:**
- **avrdude** with SerialUPDI support
- Command-line flashing workflow
- Cross-platform (Windows, macOS, Linux)

#### USB Bootloader (Vendor Protocol)
- **Type**: USB vendor-specific device (not HID)
- **Protocol**: LibUSB / WinUSB based
- **Size**: 4-6 KB (BOOTSIZE fuse in 512-byte increments)
- **Entry Method**: Firmware key combination (Fn + LeftCtrl + RightCtrl)
- **Device Name**: "AmberClick90 Bootloader" or similar vendor-specific descriptor
- **Reference Implementation**: avrusbboot (borrow ideas for vendor protocol stack)

**Bootloader Advantages:**
- No HID limitations (can use larger transfer sizes)
- LibUSB support (cross-platform)
- WinUSB on Windows (may require driver on older systems)
- Faster flashing than HID

**Host-Side Software:**
- LibUSB-based flashing tool
- WinUSB support on Windows (research driver requirements)
- Reference: avrusbboot software side

---

## 3. Firmware Architecture

### 3.1 Development Toolchain

**Core Tools:**
- **Compiler**: avr-gcc
- **Editor**: Geany (or user preference)
- **UPDI Programmer**: CH340-based SerialUPDI programmer
- **UPDI Software**: avrdude with SerialUPDI support
- **USB Flashing**: LibUSB/WinUSB-based tool (reference: avrusbboot)
- **Development Board**: AVR64DU32 Nano dev board (mouser.ca, quick delivery, affordable)

**Architecture**: Bare-metal + MCC-based USB stack + minimal HAL

**USB Stack Strategy:**
- Start with Microchip Code Configurator (MCC) USB stack
- Trim down to pure HID device Full-Speed for APPCODE (keyboard)
- Trim down to vendor protocol device for BOOTCODE (flashing mode)
- Borrow concepts from LUFA where applicable
- **NOT using TinyUSB**

### 3.2 Firmware Structure

**Repository Organization:**
```
/AmberClick90-firmware
├── src/                          # Active firmware (gets compiled)
│   ├── main.c
│   ├── bootloader.c
│   ├── keyboard.c
│   ├── matrix.c
│   ├── keymap.c                  # User's active keymap
│   ├── macros.c                  # User's active macros
│   ├── encoder.c                 # User's active encoder code
│   ├── hal_gpio.c
│   ├── hal_timer.c
│   ├── config.h
│   ├── usb_hid.c                 # HID keyboard (MCC-based, trimmed)
│   └── usb_vendor.c              # Vendor protocol bootloader (MCC-based)
├── lib/                          # Library (NOT compiled, see separate doc)
│   ├── macros/                   # Copy-paste macro library
│   ├── encoders/                 # Copy-paste encoder library
│   └── keymaps/                  # Example complete keymaps
├── mcc_generated/                # MCC USB stack (trimmed)
│   └── usb/
├── docs/
│   ├── BUILDING.md
│   ├── FLASHING.md
│   ├── CUSTOMIZATION.md
│   └── API.md
├── Makefile
└── README.md
```

**Build System:**
- Makefile compiles only files in `src/` directory
- `lib/` directory ignored by build system (reference library only)
- Users manually copy desired code from `lib/` to `src/`
- Zero unused code in compiled firmware

**Note**: See separate macro/encoder library documentation for details on the `lib/` directory structure and usage.

### 3.3 Memory Layout

#### Flash (64 KB total)

**Three-Section Architecture:**
```
|------------------------| 0x0000
| BOOTCODE Section       |
| - Vendor USB Bootloader|
| - Size: 4-6 KB         |
| - BOOTSIZE fuse        |
| - (512-byte increments)|
| - User knows how to    |
|   determine size       |
|------------------------| 0x1000 to 0x1800 (TBD after development)
| APPCODE Section        |
| - Keyboard firmware    |
| - Size: 58-60 KB       |
| - All keymaps/macros   |
| - USB HID descriptors  |
| - All configuration    |
|------------------------| 0x10000 (64KB)
| APPDATA Section        |
| - NOT USED (0 KB)      |
| - APPDATASIZE = 0x00   |
|------------------------| 0x10000
```

**Memory Section Decisions:**
- **BOOTCODE**: 4-6 KB vendor protocol USB bootloader (size determined after implementation)
- **APPCODE**: Maximum space for keyboard firmware (58-60 KB)
- **APPDATA**: Not used - all configuration is compile-time only

**Fuse Configuration:**
```c
BOOTSIZE = 0x0A to 0x0C     // 5-6 KB (user determines after bootloader compilation)
APPDATASIZE = 0x00          // 0 KB - NOT USED
```

**Determining BOOTSIZE:**
- User has method to calculate exact bootloader size
- Set BOOTSIZE = (bootloader_bytes / 512) rounded up
- Maximize APPCODE space

**Rationale for No AppData:**
- Users compile and reflash for configuration changes
- No VIA/VIAL-style runtime editing
- Simpler, more reliable firmware
- No flash wear concerns
- All configuration visible in source code
- Traditional DIY keyboard approach

#### Flash Usage Breakdown (Application Only)

| Component | Size |
|-----------|------|
| Startup + HAL (GPIO, timer) | 1.5-2 KB |
| Matrix scanning (90 keys) | 1-1.5 KB |
| Debouncing logic | 0.5-1 KB |
| Layer engine (2 main + 1 system) | 0.5 KB |
| Macro engine (simple) | 0.5-1 KB |
| Keymap data (2×90 keys) | ~360 bytes |
| Keyboard logic glue | ~1 KB |
| **Non-USB Subtotal** | **6-8 KB** |
| MCC USB HID stack (trimmed) | 8-10 KB |
| 6KRO HID class | 2-3 KB |
| USB descriptors | ~0.5 KB |
| **USB Subtotal** | **10-13 KB** |
| **Total Application** | **16-21 KB** |
| **Free Space** | **37-44 KB** |

#### SRAM (8 KB total)

| Component | Size |
|-----------|------|
| Key matrix state (90 keys) | ~90 bytes |
| Debounce buffers | ~180 bytes |
| Macro buffers | 64-128 bytes |
| USB HID reports (6KRO) | ~32 bytes |
| MCC USB buffers | 512-1 KB |
| Stack | ~1 KB |
| **Total Used** | **2-2.5 KB** |
| **Free** | **5.5-6 KB** |

### 3.4 Data Storage Strategy

**Flash (Compile-Time Constants in APPCODE):**
- Default keymaps (2 main layers × 90 keys)
- System layer (Layer 2) definitions
- Macro definitions
- USB HID descriptors
- Default LED brightness values
- Encoder action mappings
- All configuration data

**SRAM (Runtime State Only):**
- Active layer state
- Current matrix scan state
- Debounce counters
- Runtime LED brightness (loaded from flash defaults at boot)
- Macro execution state
- Current encoder position

**No Persistent User Configuration:**
- Settings do not persist across power cycles
- LED brightness returns to defaults on boot
- All customization done via source code modification and reflashing
- Runtime adjustments (Layer 2 brightness control) temporary only

**Rationale:**
- Keymaps are constants defined at compile-time
- Flash reads are single-cycle on AVR DU (no performance penalty)
- SRAM used only for runtime state (unlimited write cycles)
- Maximum simplicity and reliability
- No hidden state or configuration corruption risk
- Standard DIY keyboard workflow

### 3.5 Firmware Features

#### Layer System
- **Layer 0** (Base): Default alphanumeric layout (ergonomic, already designed)
- **Layer 1** (Fn): Function keys, media controls, macros
- **Layer 2** (System): LED brightness adjustment, bootloader entry, software reset

**Layer 2 Functions:**
- LED brightness adjustment keys (runtime only, not persistent)
- Bootloader entry combo (Fn + LeftCtrl + RightCtrl → firmware update mode)
- Software reset key (firmware restart, does NOT enter bootloader)
- Optional system configuration (runtime adjustments)

**Customization Workflow:**
1. Edit `src/keymap.c` for layer definitions
2. Edit `src/macros.c` for macro definitions (copy from `lib/macros/`)
3. Edit `src/encoder.c` for encoder behavior (copy from `lib/encoders/`)
4. Edit `src/config.h` for defaults (brightness, debounce, etc.)
5. Compile firmware (`make`)
6. Enter bootloader (Fn + LeftCtrl + RightCtrl)
7. Flash via USB vendor protocol bootloader (LibUSB tool)
8. Keyboard restarts with new configuration

#### Macro Support
- Simple macros (key sequences, modifier combinations)
- Examples: Ctrl+C, Alt+Tab with hold logic, text snippets
- Stored in flash (APPCODE section)
- Execution state in SRAM
- **Library system**: Copy implementations from `lib/macros/` (see separate documentation)

#### 6KRO (6-Key Rollover)
- Standard USB HID boot protocol compatible
- 6 simultaneous key presses + modifiers
- Simpler implementation than NKRO
- Compatible with all systems (including BIOS)
- Lower bandwidth than NKRO
- Sufficient for >99% of typing and gaming scenarios

**HID Report Format (6KRO):**
```c
typedef struct {
    uint8_t modifiers;     // Ctrl, Shift, Alt, GUI (8 bits)
    uint8_t reserved;      // Always 0
    uint8_t keys[6];       // Up to 6 simultaneous keys
} hid_keyboard_report_t;
```

#### Encoder Features
- Quadrature decoding (state machine or table-driven)
- Macro dispatch on rotation
- Layer-dependent behavior
- **Smart behaviors**: Alt-Tab with hold logic, volume with acceleration, etc.
- **Library system**: Copy implementations from `lib/encoders/` (see separate documentation)

#### Debouncing
- **Matrix**: 5-20ms typical (tune during testing)
- **Encoder**: 1-2ms (lighter due to increased mechanical bounce)
- Software-based debouncing

#### Out of Scope (Explicitly Not Implementing)
- ❌ **USB MIDI** - Scoped out (gimmick)
- ❌ **Mouse emulation** - Scoped out (gimmick)
- ❌ **On-the-fly macro recording** - Scoped out (gimmick)
- ❌ **NKRO** - Using 6KRO instead (simpler, sufficient)

### 3.6 USB Device Configuration

**Application Mode (APPCODE):**
- **Device Class**: HID (Human Interface Device)
- **Device Name**: "AmberClick90"
- **Protocol**: 6KRO keyboard (boot protocol compatible)
- **Manufacturer**: [User's name/handle]
- **Version**: "AC90-r1" (or semantic versioning)

**Bootloader Mode (BOOTCODE):**
- **Device Class**: Vendor-specific (not HID)
- **Device Name**: "AmberClick90 Bootloader"
- **Protocol**: LibUSB / WinUSB based
- **Reference**: avrusbboot protocol

**USB Stack:**
- Based on Microchip Code Configurator (MCC)
- Trimmed for minimal code size
- LUFA concepts borrowed where applicable
- NOT using TinyUSB

---

## 4. Bootloader Implementation

### 4.1 Bootloader Type: Vendor Protocol (LibUSB/WinUSB)

**Specifications:**
- Flash size: 4-6 KB (user determines after development)
- SRAM: 512-1024 bytes
- Enumeration: USB vendor-specific device
- Protocol: Custom (reference: avrusbboot)
- LibUSB on Linux/macOS, WinUSB on Windows

**Advantages:**
- ✓ Larger transfer sizes than HID (faster flashing)
- ✓ Cross-platform LibUSB support
- ✓ No HID limitations
- ✓ Simpler than HID for bulk transfers

**Host-Side Software:**
- LibUSB-based flashing tool
- May require WinUSB driver on Windows (research requirements)
- Command-line or simple GUI
- Reference: avrusbboot software implementation

### 4.2 Bootloader Entry Methods

#### Primary: Firmware Key Combination (Bootloader Entry)
- **Combo**: Fn + LeftCtrl + RightCtrl
- **Location**: Layer 0 or Layer 1 (via Fn key)
- **Detection**: During normal matrix scanning with debouncing
- **Process**: Firmware writes GPR magic value, triggers watchdog reset
- **Purpose**: Enter firmware update mode (USB vendor protocol bootloader)

#### Secondary: Software Reset Key (Firmware Restart Only)
- **Location**: Layer 2 (system layer)
- **Process**: Watchdog reset without setting GPR magic value
- **Purpose**: Quick firmware restart (does NOT enter bootloader)
- **Behavior**: Normal firmware startup after reset

#### Tertiary: Encoder Push Switch
- **Connection**: RESET pin (hardware)
- **Behavior**: Hardware reset, firmware restart
- **Note**: Does NOT enter bootloader (unless GPR flag happens to be set)

#### Recovery: UPDI Programming
- **Method**: Direct flash programming via UPDI interface
- **Programmer**: CH340-based SerialUPDI
- **Software**: avrdude with SerialUPDI support
- **Purpose**: Unbrick corrupted firmware or bootloader

### 4.3 Bootloader Request Flow

#### GPR Boot Flag Mechanism

**Magic Value Storage:**
- Use General Purpose Register r2 (or r3)
- Magic value: `0x42` (or other distinctive pattern)
- Survives watchdog reset (confirmed behavior)
- May survive RESET pin and direct software reset (test on dev board)
- Does NOT survive power-on or brown-out reset

**Bootloader Entry (Fn + LeftCtrl + RightCtrl):**

```c
#define BOOT_MAGIC 0x42

// Step 1: Detect key combination in matrix scan
if (fn_layer_active &&
    key_is_pressed(KEY_LEFT_CTRL) &&
    key_is_pressed(KEY_RIGHT_CTRL)) {
    request_bootloader();
}

// Step 2: Set boot flag and reset
void request_bootloader(void) {
    asm volatile (
        "ldi r2, %0\n"
        :
        : "M" (BOOT_MAGIC)
    );
    wdt_enable(WDTO_15MS);
    while (1) {}  // Wait for watchdog reset
}

// Step 3: Early boot check (before C runtime init)
if (r2 == BOOT_MAGIC) {
    r2 = 0;  // Clear immediately to prevent false positives
    stay_in_bootloader();
} else {
    jump_to_application();
}
```

**Software Reset (Layer 2 Key):**

```c
// Does NOT set boot flag - firmware restarts normally
void software_reset(void) {
    // GPR r2 left unchanged (no magic value)
    wdt_enable(WDTO_15MS);
    while (1) {}
}
```

### 4.4 Reset Behavior Summary

| Reset Type | GPR Preserved? | Boot Flag | Result |
|------------|----------------|-----------|--------|
| Power-on | ❌ No | N/A | Normal firmware start |
| Brown-out | ❌ No | N/A | Normal firmware start |
| Watchdog (with flag) | ✅ Yes | Present | Enter bootloader |
| Watchdog (no flag) | ✅ Yes | Absent | Normal firmware start |
| RESET pin (encoder) | ❓ Test on dev board | N/A* | Normal firmware start* |
| Software Reset (L2) | ✅ Yes** | Absent | Normal firmware start |

\* Assumes no boot flag set; test GPR behavior on dev board  
\*\* Assumes watchdog-based reset

**Key Distinction:**
- **Bootloader Entry**: Sets boot flag → enters firmware update mode
- **Software Reset**: No boot flag → restarts firmware normally
- **Encoder Button**: Hardware reset → firmware restart (not bootloader)

### 4.5 Bootloader State Machine

```
Power On / Reset
    ↓
Bootloader Starts
    ↓
Check GPR r2 for BOOT_MAGIC
    ↓
   [Magic == 0x42?]
    ↓              ↓
   Yes            No
    ↓              ↓
Stay in         Jump to
Bootloader      Application
(USB Vendor)    (Keyboard)
```

**User-Visible Behavior:**

| Scenario | Boot Flag? | Result |
|----------|-----------|--------|
| Normal power-on | No | Keyboard starts immediately |
| Fn+LeftCtrl+RightCtrl | Yes | Enter bootloader (FW update mode) |
| Layer 2 software reset | No | Firmware restarts normally |
| Encoder button (RESET) | No* | Firmware restarts normally |
| UPDI programming | N/A | Direct flash write (bypass bootloader) |

\* Assumes GPR behavior on RESET pin (test on dev board)

### 4.6 Bootloader Protection

**Fuse Configuration:**
- BOOTSIZE fuse reserves bootloader section (512-byte increments)
- User determines exact size after bootloader compilation
- Bootloader protected from application overwrites
- Application cannot write to bootloader section
- UPDI always enabled (NEVER disable)

**Jump Mechanism:**
```c
#define BOOTLOADER_ADDR 0x0000  // Adjust per BOOTSIZE fuse

void jump_to_application(void) {
    cli();  // Disable interrupts
    // Jump to application start
    ((void (*)(void))(BOOTLOADER_ADDR + bootloader_size))();
}
```

### 4.7 USB Enumeration Behavior

**From Host Perspective:**

1. **Normal Operation**: 
   - Device: "AmberClick90" keyboard
   - HID device class (6KRO)

2. **Bootloader Entry (Fn+Ctrl+Ctrl)**: 
   - Keyboard disconnects
   - New device appears: "AmberClick90 Bootloader"
   - Vendor-specific device class
   - LibUSB/WinUSB protocol

3. **Firmware Update**: 
   - Host tool sends firmware via vendor protocol
   - Bootloader writes to flash

4. **Update Complete**: 
   - Device resets
   - Re-enumerates as "AmberClick90" keyboard (HID)

5. **Software Reset (Layer 2)**: 
   - Brief USB disconnect/reconnect
   - Same device (keyboard mode, HID)

---

## 5. Component Selection

### 5.1 Bill of Materials (BOM)

| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| **MCU** | AVR64DU32-I/PT or similar | 1 | TQFP-32, use internal oscillator |
| **Dev Board** | AVR64DU32 Nano | 1 | mouser.ca, quick delivery, affordable |
| **UPDI Programmer** | CH340-based SerialUPDI | 1 | Works with avrdude |
| **USB Connector** | USB Mini-B, through-hole | 1 | Full-Speed capable, board-mount |
| **UPDI Header** | 0.1" pitch, 3-pin | 1 | GND, UPDI, VDD (verify datasheet) |
| **USB Series Resistors** | 22-33Ω, 0805 | 2 | For D+, D- |
| **USB Bulk Cap** | 4.7-10µF, 0805 or larger | 1 | Near connector, X5R/X7R |
| **VDD Decoupling Caps** | 100nF, 0805 | 5-8 | One per VDD pin |
| **VUSB Decoupling Cap** | 100nF, 0805 | 1 | Near VUSB pin |
| **Polyfuse** | 500mA, 1812 or radial | 1 | Between VBUS and VDD |
| **ESD Protection** | Low-cap USB diode array | 1 | Optional (USBLC6-2SC6 or similar) |
| **Switches** | Matias Click or Quiet Click | 90 | Identical footprints & plates |
| **Keycaps** | Alps-compatible set | 90+ | See matias.ca for sources |
| **Diodes** | 1N4148W, SOD-123 | 100 | One per switch + spares |
| **Rotary Encoder** | ALPS EC11 with switch | 1



## Prompt:
12/27/2025, 11:30:44 PM

Bootloader should be accessed via Fn (tap) -Fn (hold)-LeftCtrl (hold)-RightCtrl (hold). provide a suggested matrix scanning frequency : if yOu agree with the 1khz = 1ms to 2khz = 0.5ms, explain why (just hEurostics?).



## Response:
12/27/2025, 11:33:48 PM

| With detent clicks |
| **Power LED** | 3mm amber, diffused, TH | 1 | Fixed brightness |
| **Indicator LEDs** | 3mm amber, diffused, TH | 2 | PWM-controlled |
| **Power LED Resistor** | ~6kΩ, 0805 or 1/4W TH | 1 | Test for desired brightness |
| **Indicator LED Resistors** | 270Ω, 0805 or 1/4W TH | 2 | Near LEDs |

**Additional Components:**
- Solder (63/37 or 60/40 leaded, 0.5-0.8mm diameter)
- Flux (rosin core or pen)
- Isopropyl alcohol (for cleanup)

### 5.2 Soldering Equipment

**Required for Assembly:**
- Soldering iron: Hakko FX-888DX (or equivalent temperature-controlled)
- Recommended tip: **Hakko T18-C2** (2mm conical) or **T18-D12** (1.2mm chisel)
- Temperature: 290-315°C (550-600°F) for leaded solder
- Fine-point tweezers (ESD-safe preferred)
- Magnification: 2.5x-5x (helping hands or magnifying lamp)
- Flush cutters (for LED/component leads)

**SOD-123 Diode Hand-Soldering:**
- Package size: 3.5mm × 1.6mm × 1.1mm (easily hand-solderable)
- Technique: Pre-tin one pad, tack diode, solder other side, re-flow first side
- Time: ~20-30 seconds per diode
- Total for 90 diodes: ~30-45 minutes

---

## 6. PCB Design Guidelines

### 6.1 Physical Layout

**Component Placement:**
- **USB connector**: Top edge, aligned appropriately with ergonomic layout
- **MCU**: As close to USB connector as possible (minimize D+/D- trace length)
- **Rotary encoder**: Accessible position (user-defined in ergonomic layout)
- **Three LEDs**: Horizontal arrangement near top edge (visible, non-intrusive)
- **UPDI header**: Top edge (easier case design) or alternate edge (cleaner routing)
- **Switches**: **Ergonomic layout already designed** (90 keys, positions finalized)

**Silkscreen:**
- Project name: "AmberClick90"
- Trim designation: "(AVRDU.Enc.Mat)"
- Typographic logo (TBD - minimalist design)
- Component reference designators
- Cathode markings on all diodes (consistent orientation)
- Pin 1 indicators on MCU and connectors
- UPDI pin labels (3-pin header)
- Monochrome aesthetic (white on black)

### 6.2 Design Specifications

- **PCB**: 2-layer, black solder mask, white silkscreen
- **Thickness**: Standard 1.6mm
- **Surface Finish**: ENIG or HASL (lead-free)
- **Plate**: Compatible with Matias Click and Matias Quiet Click (identical requirements)
- **Case**: Minimal, grey 3D-printed (research filament options)
- **Aesthetic**: Minimalist, technical/instrument-like

### 6.3 Net Naming Convention

**Power:**
- VDD (5V)
- VUSB (3.3V internal)
- GND (single plane, no separate analog ground)

**Matrix:**
- ROW_0 through ROW_9
- COL_0 through COL_8

**Encoder:**
- ENC_A, ENC_B
- RESET (encoder switch and MCU pin)

**LEDs:**
- LED_PWR (power LED)
- LED_IND_A, LED_IND_B (PWM indicators)

**USB:**
- USB_DP, USB_DM
- VBUS

**Programming:**
- UPDI (3-pin header: GND, UPDI, VDD)
- RESET

### 6.4 Critical Routing

**USB Differential Pair (Highest Priority):**
- Keep D+ and D- as short, matched-length traces
- Route as differential pair (~90Ω impedance)
- Minimize stubs (ideally zero)
- Avoid vias if possible (use if necessary, minimize stub length)
- Keep away from high-frequency signals (matrix scanning)
- Ground plane clearance appropriate for impedance

**Power Distribution:**
- Wide traces for VDD/GND (0.5-1mm minimum for VDD)
- Single ground plane on bottom layer (no splits)
- Star-ground topology from USB connector
- Polyfuse placed immediately after USB connector
- Decoupling caps as close to IC pins as possible

**Matrix Routing:**
- Keep row/column traces away from USB differential pair
- Orient diodes consistently (all cathodes same direction)
- Silkscreen cathode band on all positions
- Allow ~2mm clearance around diode pads for soldering access
- Route according to ergonomic layout (already designed)

**Diode Footprint (SOD-123):**
- IPC-7351 standard footprint
- Pad size: 1.4mm × 1.0mm
- Pad spacing: 3.5mm center-to-center
- Silkscreen cathode indicator (line or arrow)

**LED Placement:**
- Through-hole pads sized for 3mm LED leads
- Resistors near LEDs (not MCU)
- Polarity markings on silkscreen (flat edge, cathode)

---

## 7. Firmware Development Roadmap

### 7.1 Phase 1: Research & Planning (Weeks 1-2)

**Critical Research (Week 1):**
1. **MCC USB Stack**:
   - Generate initial MCC USB project for AVR64DU32
   - HID device Full-Speed configuration for APPCODE
   - Vendor protocol device configuration for BOOTCODE
   - Trim unnecessary code for minimal footprint

2. **avrusbboot Reference**:
   - Study vendor protocol implementation (bootloader side)
   - Study LibUSB/WinUSB tool (host side)
   - Adapt protocol for AmberClick90

3. **LUFA Concepts**:
   - Review relevant LUFA USB handling
   - Borrow ideas for MCC stack enhancement

4. **Component Verification**:
   - ✅ AVR64DU32 Nano dev board (mouser.ca, confirmed available)
   - ✅ CH340-based SerialUPDI programmer (confirmed works with avrdude)
   - Matias Click/Quiet Click switch stock levels
   - Alps-compatible keycap availability
   - Lead times for long-lead items

5. **Thermal Considerations**:
   - Research AVR64DU32 thermal characteristics at full-speed (24 MHz)
   - No power constraints (hardwired to PC), but monitor heat
   - Test on dev board under continuous load

**Parallel Activities (Weeks 1-2):**
- Logo design (minimalist, silkscreen-friendly)
- 3D case filament research (grey colors available)
- Test environment setup (Geany, avr-gcc, make, avrdude)

### 7.2 Phase 2: Development Environment Setup (Weeks 2-3)

1. **Acquire Hardware**:
   - ✅ AVR64DU32 Nano dev board (order from mouser.ca)
   - ✅ CH340-based SerialUPDI programmer
   - Basic components for testing (LEDs, resistors)

2. **Toolchain Setup**:
   - Install avr-gcc (verify AVR DU support)
   - Configure Geany for AVR development
   - Install and test avrdude with SerialUPDI support
   - Test UPDI programming (blink test on dev board)

3. **MCC Project Setup**:
   - Generate MCC USB project for AVR64DU32
   - Configure HID device (6KRO keyboard)
   - Configure vendor protocol device (bootloader)
   - Trim generated code for minimal size
   - Build and flash test firmware via UPDI

4. **Minimal HAL Development**:
   - GPIO abstraction (port-based access for AVR64DU32)
   - TCA PWM setup (test with LEDs on dev board)
   - Watchdog configuration and reset behavior
   - Test GPR preservation across reset types

5. **Library Structure**:
   - Create `lib/macros/` directory structure
   - Write initial macro examples (copy/paste, volume)
   - Write initial encoder examples (basic volume, smart Alt-Tab)
   - Document API in `docs/API.md`

### 7.3 Phase 3: Schematic Design (Weeks 3-4)

**Schematic Organization (KiCad):**

1. **Page 1 - Power**:
   - USB connector
   - Polyfuse
   - Bulk capacitor
   - VDD distribution
   - Symbol for VUSB

2. **Page 2 - MCU**:
   - AVR64DU32 with all pin assignments
   - VDD/VUSB decoupling capacitors
   - 3-pin UPDI header (GND, UPDI, VDD)
   - RESET connection (encoder switch)

3. **Page 3 - USB Interface**:
   - USB Mini-B connector pinout
   - D+/D- series resistors
   - Optional ESD protection
   - USB shield connection

4. **Page 4 - Key Matrix**:
   - Row/column connections to MCU
   - Diode placement (SOD-123, one per switch)
   - Switch footprints (Matias Click/Quiet Click compatible)
   - Ergonomic layout positions (already designed)

5. **Page 5 - Peripherals**:
   - Encoder connections (A, B, switch to RESET)
   - Three LEDs with resistors
   - PWM connections (PA4, PA5)

**Deliverable**: Complete schematic with DRC passed, ready for layout

### 7.4 Phase 4: Firmware Development on Dev Board (Weeks 4-6)

**Parallel with PCB Design/Fabrication:**

1. **Bootloader Implementation**:
   - GPR boot flag mechanism (test watchdog reset preserves r2)
   - **Test RESET pin behavior on dev board** (verify GPR preservation)
   - Vendor protocol USB stack (MCC-based, reference avrusbboot)
   - Flash write routines
   - Application jump logic
   - USB vendor device enumeration

2. **Host-Side Flashing Tool**:
   - LibUSB-based tool (reference avrusbboot)
   - Test on Linux/macOS (LibUSB)
   - Test on Windows (WinUSB, research driver requirements)
   - Command-line interface

3. **Core Keyboard Firmware**:
   - Matrix scanning (simulated on dev board with 3×3 test matrix)
   - Debouncing algorithm (tune timing)
   - Layer switching logic (3 layers)
   - 6KRO HID descriptor and reporting

4. **Encoder Emulation**:
   - Quadrature decoder with two buttons on dev board
   - Test "smart" Alt-Tab behavior with timeout
   - Verify encoder state initialization

5. **LED Control**:
   - TCA PWM configuration (PA4, PA5)
   - 8-bit brightness control
   - Verify no USB timing interference

### 7.5 Phase 5: PCB Design (Weeks 5-7)

**PCB Layout (KiCad):**

1. **Board Setup**:
   - Import ergonomic layout (90 keys, positions already designed)
   - Define board outline based on layout
   - Set design rules (trace width, clearance, via size)

2. **Critical Component Placement**:
   - USB connector (top edge)
   - MCU (as close to USB as possible)
   - Encoder (per ergonomic layout)
   - LEDs (horizontal arrangement, top edge)
   - 3-pin UPDI header (top or alternate edge)

3. **Switch Placement**:
   - Use ergonomic layout (already designed, 90 keys)
   - Verify Matias Click/Quiet Click footprint compatibility
   - Ensure plate mounting compatibility

4. **Critical Routing (in order)**:
   - USB differential pair (D+/D-, matched length, 90Ω, no vias)
   - Power traces (VDD, GND, wide traces, star ground)
   - Matrix rows and columns (per ergonomic layout)
   - Encoder signals
   - LED connections
   - Remaining signals

5. **Ground Plane and Finishing**:
   - Pour ground plane on bottom layer
   - Add thermal reliefs for through-hole pads
   - Silkscreen (logo, labels, component markings, 3-pin UPDI labels)
   - Design rule check (DRC)
   - Electrical rule check (ERC)

6. **Design Review**:
   - Verify all net connections
   - Check component footprints (especially Matias switches)
   - Verify polarity markings
   - Review silkscreen clarity

**Deliverable**: PCB ready for fabrication (Gerber files)

### 7.6 Phase 6: Fabrication & Procurement (Weeks 6-10)

**PCB Fabrication (Weeks 6-8)**:
- Submit Gerber files to manufacturer (JLCPCB, PCBWay, etc.)
- Lead time: 2-4 weeks typical
- Order 5-10 boards (allow for mistakes/iterations)

**Component Ordering (Week 6)**:

*Order immediately after schematic finalized:*
- AVR64DU32 (verify stock)
- Standard passives (caps, resistors - order extras)
- USB connector, 3-pin UPDI header, polyfuse
- Encoder, LEDs

*Order after PCB layout validated:*
- Matias Click or Quiet Click switches (90+ units, verify stock)
- Alps-compatible keycaps (verify availability)
- SMD diodes 1N4148W SOD-123 (100 units)

**During Fabrication**:
- Continue firmware development on dev board
- Test thermal behavior under load
- Prepare assembly workspace
- Verify soldering equipment ready
- Practice SOD-123 soldering technique

### 7.7 Phase 7: Assembly & Testing (Weeks 8-10)

**Assembly Order:**

1. **SMD Diodes (90×)** - FIRST, before anything else:
   - Flat PCB easier to work with
   - No components blocking access
   - Use PCB edge for support
   - Orient all diodes consistently (per ergonomic layout)
   - Visual inspection after completion

2. **MCU**:
   - Careful with TQFP-32 orientation (pin 1)
   - Flux and drag soldering technique
   - Inspect for bridges

3. **Passive Components**:
   - Resistors and capacitors
   - Near respective ICs

4. **USB Connector and 3-pin UPDI Header**:
   - Through-hole, straightforward
   - Verify UPDI pinout (GND, UPDI, VDD)

5. **LEDs**:
   - Mind polarity (flat edge = cathode)
   - Through-hole, last SMD-related components

6. **Initial Testing** (before switches):
   - Visual inspection (no shorts, proper orientation)
   - Continuity testing (VDD, GND, critical nets)
   - Power-on test (measure VDD = 5V, VUSB = 3.3V)
   - UPDI programming test (flash blink test via avrdude + SerialUPDI)
   - USB enumeration test (device appears as HID keyboard)
   - Basic GPIO test (toggle pins, test LEDs)
   - **Thermal test** (run at full speed, monitor MCU temperature)

7. **Encoder**:
   - After basic tests pass
   - Verify mechanical fit

8. **Switches** (only after firmware working):
   - Install all 90 switches (per ergonomic layout)
   - Test continuity of each position
   - Verify no shorts

**Full System Testing**:
- All 90 keys register correctly (per ergonomic layout)
- 6KRO verification (6 simultaneous keys + modifiers)
- Layer switching (all 3 layers)
- Encoder rotation and detents
- Encoder push (reset)
- LED brightness control (PWM)
- Bootloader entry (Fn + LeftCtrl + RightCtrl, vendor protocol)
- Software reset (Layer 2 key)
- Macro execution
- Firmware update via USB vendor protocol (LibUSB tool)
- Cross-platform testing (Windows, macOS, Linux)

### 7.8 Phase 8: Case Design & Final Assembly (Weeks 9-12)

1. **Case Design** (Week 9-10):
   - Wait for assembled PCB to verify dimensions
   - Measure actual PCB dimensions
   - CAD model based on ergonomic layout
   - Account for USB connector clearance
   - Account for LED visibility
   - Encoder knob design
   - Plate design (compatible with Matias Click/Quiet Click)

2. **Test Print** (Week 10):
   - Print small section to verify tolerances
   - Test fit with PCB
   - Check switch clearances

3. **Final Print** (Week 11):
   - Select grey filament (based on research)
   - Full case print
   - Plate print/fabrication
   - Fit test with fully assembled PCB
   - Verify keycap clearances

4. **Final Assembly & Tuning** (Week 12):
   - Install keycaps (Alps-compatible)
   - Fine-tune firmware (debounce, brightness, encoder sensitivity)
   - Burn-in testing (extended use, thermal monitoring)
   - Documentation (build notes, photos)

**Total Estimated Timeline**: 10-13 weeks from start to completion

---

## 8. Firmware Configuration Details

### 8.1 Clock Configuration

- **Primary Clock**: 24 MHz internal oscillator (full-speed operation)
- **USB Clock**: Derived via USB clock recovery (SOF-based)
- **Power Mode**: Full-speed continuous (no low-power modes needed)
- **Thermal Monitoring**: Test MCU temperature under load on dev board

### 8.2 Fuse Settings

**Must Configure (Critical):**
```c
// Memory sections
BOOTSIZE = [User determines]   // 4-6 KB bootloader (calculated after compilation)
                               // Formula: (bootloader_bytes / 512) rounded up
APPDATASIZE = 0x00            // 0 KB - NOT USED

// Protection and features
UPDI: Enabled                 // NEVER DISABLE (3-pin header: GND, UPDI, VDD)
BOD: [TBD - Test on dev board] // Brown-out detection level
WDT: [TBD - Test on dev board] // Watchdog configuration
RSTPIN: Enabled               // Reset pin (encoder switch)
```

**User Has Method to Determine BOOTSIZE:**
- Compile bootloader
- Calculate: BOOTSIZE = (bootloader_size_bytes / 512) rounded up
- Set fuse accordingly
- Maximize APPCODE space

### 8.3 Matrix Scanning Parameters

**Configuration (Tune During Testing):**
- **Scan rate**: 1-2 kHz typical
- **Debounce time**: 5-20ms (start at 10ms, adjust based on testing)
- **Pull configuration**: Research pull-up vs pull-down (test on dev board)
- **Diode orientation**: Determine during schematic (COL2ROW vs ROW2COL)

**Port Optimization:**
- Rows grouped where possible → single register read
- Minimize port switching during scan
- Use bitmasks for efficient row/column access

### 8.4 PWM Configuration

**Timer: TCA (Timer Counter A)**
- Single timer drives both PWM channels (PA4, PA5)

**Parameters:**
- **Frequency**: 1-2 kHz (matrix scan also 1-2 kHz, verify no aliasing)
- **Resolution**: 8-bit (0-255)
- **Prescaler**: Calculate based on 24 MHz clock
- **Waveform**: Fast PWM mode

**Brightness Control:**
```c
// Flash (compile-time defaults)
#define LED_BRIGHTNESS_DEFAULT 128  // 50% brightness

// SRAM (runtime, lost at power-off)
uint8_t led_brightness_a;
uint8_t led_brightness_b;

// At startup:
led_brightness_a = LED_BRIGHTNESS_DEFAULT;
led_brightness_b = LED_BRIGHTNESS_DEFAULT;

// Layer 2 keys adjust runtime values
```

### 8.5 Encoder Configuration

**Decoder Type:**
- State machine (preferred) or table-driven
- Handles mechanical bounce
- Detects forward/reverse rotation

**Sampling:**
- Poll during matrix scan loop (1-2 kHz)
- Or pin-change interrupt (optional optimization)

**Integration:**
- Encoder state updated in main loop
- Dispatch to macro functions based on direction and layer
- Timeout handling for "smart" behaviors (e.g., Alt-Tab)

**Example Action Mapping:**
```c
// Stored in Flash (APPCODE section)
const encoder_action_t encoder_map[] = {
    [LAYER_0] = { .cw = MACRO_VOL_UP,    .ccw = MACRO_VOL_DOWN },
    [LAYER_1] = { .cw = MACRO_ALT_TAB,   .ccw = MACRO_ALT_TAB_REV },
    [LAYER_2] = { .cw = MACRO_BRIGHT_UP, .ccw = MACRO_BRIGHT_DOWN }
};
```

---

## 9. Design Philosophy & Best Practices

### 9.1 Memory Management

**Flash Usage:**
- Store all constant data (keymaps, macros, descriptors) in APPCODE section
- No runtime writes (except via bootloader during firmware update)
- User determines BOOTSIZE to maximize APPCODE space
- Design allows scaling to future AVR-DU variants with >64 KB flash

**SRAM Usage:**
- Minimize static allocation
- Use only for runtime state (unlimited write cycles)
- Monitor stack usage (allocate ~1 KB safety margin)
- Design allows scaling to future AVR-DU variants with >8 KB SRAM

### 9.2 Code Quality

**Style:**
- Direct register access where performance-critical
- Minimal abstraction overhead
- Clean HAL for portability across AVR-DU family
- Well-commented for maintainability

**Portability:**
- Borrow proven concepts from MCC, LUFA, avrusbboot
- Adapt specifically for AVR-DU architecture
- Document deviations from reference implementations
- Code structure allows future porting to larger AVR-DU variants

**Target Platform:**
- Firmware developed specifically for AVR64DU32
- Best MCU in AVR-DU family currently available
- No backward compatibility with smaller AVR-DU parts
- Forward compatibility via clean architecture, not #ifdef complexity

### 9.3 User Experience

**Boot Time:**
- Target: <100ms from power-on to functional keyboard
- Quick bootloader check with no perceptible delay

**Responsiveness:**
- Low-latency matrix scanning (1-2ms scan cycle)
- Immediate key feedback (no lag)
- Smooth encoder operation (no missed detents)

**Reliability:**
- Robust debouncing (no false triggers, no missed presses)
- Stable USB enumeration (works on first plug-in)
- Predictable behavior (no race conditions)

**Customization:**
- Clear source code organization
- Well-documented macro/encoder library
- Easy to modify and reflash via USB vendor protocol
- Git-friendly workflow for version control

### 9.4 Development Principles

**YAGNI (You Aren't Gonna Need It):**
- No APPDATA section (not needed for target use case)
- No VIA/VIAL runtime editing (compile-time is sufficient)
- No complex build system (simple Makefile)
- No NKRO (6KRO is sufficient)
- No USB MIDI, mouse emulation, or on-the-fly macro recording (gimmicks)

**KISS (Keep It Simple):**
- Copy-paste macro library (no preprocessor complexity)
- Direct register access (no HAL bloat)
- Single-purpose functions (easy to understand)
- 6KRO instead of NKRO (simpler, boot protocol compatible)

**Community-Oriented:**
- Open source firmware and documentation
- Shareable macro/encoder library
- Contribution-friendly structure

### 9.5 Thermal Considerations

**Full-Speed Operation:**
- 24 MHz continuous (no power-saving modes)
- Hardwired to PC (110V outlet, no battery constraints)
- **Monitor thermal implications**:
  - Test on dev board under continuous load
  - Measure MCU temperature
  - Verify operation within safe limits
  - Add heatsink if needed (unlikely for keyboard application)

---

## 10. Testing & Validation

### 10.1 Hardware Testing Checklist

**Bare PCB (Before Switch Assembly):**
- [ ] Visual inspection (no shorts, solder bridges, proper orientation)
- [ ] Continuity testing (VDD to all VDD pins, GND continuity)
- [ ] Power-on test (measure VDD = 5V ±5%, VUSB = 3.3V ±5%)
- [ ] Polyfuse test (verify ~500mA trip point)
- [ ] UPDI programming (flash blink test via avrdude + SerialUPDI, 3-pin header)
- [ ] USB enumeration (device appears as "AmberClick90" HID keyboard)
- [ ] Basic GPIO test (toggle test pins, verify with multimeter)
- [ ] LED test (all 3 LEDs light up, check polarity)
- [ ] PWM test (verify brightness control on indicators)
- [ ] **Thermal test** (run at 24 MHz full-speed, monitor MCU temperature)

**After Switch Assembly:**
- [ ] All 90 switches register correctly (per ergonomic layout)
- [ ] No ghost keys (test problematic combinations)
- [ ] Encoder rotation (both directions, all detents)
- [ ] Encoder push (reset functionality, firmware restarts)

### 10.2 Firmware Testing Checklist

**Core Functionality:**
- [ ] Matrix scanning accuracy (every key detected, ergonomic layout)
- [ ] Debouncing effectiveness (no chattering, no missed presses)
- [ ] Layer switching (all 3 layers accessible and functional)
- [ ] 6KRO verification (6 keys + modifiers simultaneously)
- [ ] Encoder decoding accuracy (all rotations detected, no phantom events)
- [ ] Encoder direction correct (CW/CCW match expected behavior)

**Special Functions:**
- [ ] LED brightness control (PWM working, visible brightness change)
- [ ] Layer 2 brightness adjustment (runtime control functional)
- [ ] Bootloader entry (Fn + LeftCtrl + RightCtrl → enters vendor protocol bootloader)
- [ ] Software reset (Layer 2 key → firmware restarts normally)
- [ ] Encoder reset (push switch → firmware restarts)
- [ ] Macro execution (test various macros from library)
- [ ] **GPR preservation** (test on dev board: watchdog, RESET pin, software reset)

**USB Functionality:**
- [ ] Firmware update via USB vendor protocol (LibUSB tool)
- [ ] USB stability (no disconnects during normal use)
- [ ] Cross-platform compatibility (Windows, macOS, Linux)
- [ ] WinUSB driver requirements on Windows (document)

### 10.3 Power Testing

**Current Draw Measurements:**
- [ ] Idle current (no keys pressed, default LED brightness)
- [ ] Active scanning (matrix scanning, encoder polling, full-speed 24 MHz)
- [ ] All LEDs at 100% brightness
- [ ] 6 keys pressed simultaneously (6KRO stress test)
- [ ] Peak current during USB enumeration

**Thermal Testing:**
- [ ] MCU temperature at idle (24 MHz)
- [ ] MCU temperature under load (continuous matrix scanning, LEDs on)
- [ ] Extended operation test (1+ hour, monitor temperature)
- [ ] Verify safe operating temperature range

**Targets:**
- Idle: <50 mA
- Active (typical use): 50-150 mA
- All LEDs + matrix: <300 mA
- Peak (everything on): <500 mA (must not trip polyfuse)
- MCU temperature: <70°C (stay well below absolute maximum)

### 10.4 Long-Term Testing

- [ ] Burn-in test (24-hour continuous operation, thermal monitoring)
- [ ] Key endurance (repeated presses on high-use keys)
- [ ] Thermal stability (verify no thermal throttling or drift)
- [ ] USB cable stress (multiple plug/unplug cycles)

---

## 11. Future Enhancements (Optional)

**Advanced Features (If Desired Later):**
- RGB underglow (addressable LEDs on PCB underside)
- Audio output (piezo buzzer for keypress feedback)
- Additional encoders (dual-encoder layout variant)
- OLED display (status, layer indicator, WPM counter)
- Per-key RGB backlighting (if Matias switches with clear housing available)

**Platform Scaling:**
- Port to future AVR-DU variants with more resources
- Larger keyboard layouts (100-key, full-size)
- Split keyboard variant (AmberClick90 Split)

**Explicitly Out of Scope (Not Implementing):**
- ❌ USB MIDI support (gimmick)
- ❌ Mouse emulation (gimmick)
- ❌ On-the-fly macro recording (gimmick)
- ❌ NKRO (6KRO is sufficient and simpler)
- ❌ VIA/VIAL runtime keymap editing (users reflash for changes)
- ❌ Wireless connectivity (Bluetooth)
- ❌ Battery power (USB-powered only)
- ❌ APPDATA section usage (all configuration compile-time)

---

## 12. References & Resources

### 12.1 Datasheets

- [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)
- [ALPS EC11 Encoder Datasheet](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)
- [Matias Switch Datasheet](https://matias.ca/switches/datasheet.pdf)
- [Matias Click Switch & Keycaps](https://matias.ca/switches/click/)
- [1N4148W Diode Datasheet](https://www.diodes.com/assets/Datasheets/ds30086.pdf)
- USB 2.0 Specification

### 12.2 Software & Tools

- **Microchip Code Configurator (MCC)** (USB stack generation)
- **avrusbboot** (reference for vendor protocol bootloader)
- **LUFA** (USB concepts reference)
- **avr-gcc** (GCC compiler for AVR)
- **avrdude** (firmware flashing tool with SerialUPDI support)
- **SerialUPDI** (CH340-based UPDI programmer)
- **LibUSB** (cross-platform USB library)
- **WinUSB** (Windows USB driver)
- **KiCad** (PCB design)
- **Geany** (text editor for firmware development)

### 12.3 Hardware

- **AVR64DU32 Nano dev board** (mouser.ca)
- **CH340-based SerialUPDI programmer** (compatible with avrdude)

### 12.4 Standards & Specifications

- USB HID Usage Tables (6KRO keyboard scancodes)
- USB 2.0 Full-Speed Specification
- USB Vendor-Specific Device Class
- IPC-7351 (SMD footprint standards)
- IPC-2221 (PCB design standards)

### 12.5 Community Resources

- **Keyboard Layout Editor** (keyboard-layout-editor.com) - Layout visualization
- **Git** - Version control for personal configurations
- **GitHub** - Firmware repository, issue tracking, discussions
- **r/MechanicalKeyboards** - Community support and inspiration

### 12.6 Firmware Customization

See separate macro/encoder library documentation for:
- Copy-paste macro library (`lib/macros/`)
- Copy-paste encoder library (`lib/encoders/`)
- Example keymaps (`lib/keymaps/`)
- API reference (`docs/API.md`)
- Customization guide (`docs/CUSTOMIZATION.



## Prompt:
12/28/2025, 8:31:00 AM

Continue



## Response:
12/28/2025, 8:32:37 AM

md`)

---

## 13. Typographic Logo Design

**Requirements:**
- Simple, silkscreen-friendly design
- Minimalist aesthetic matching project philosophy
- Technical/instrument-like appearance
- Monochrome (white silkscreen on black PCB)
- Incorporates "AmberClick90" or "AC90" branding
- Readable at PCB scale (~5-10mm height)

**Design Constraints:**
- Minimum line width: 0.15mm (typical silkscreen capability)
- No fine serifs or intricate details
- Clear letterforms at small sizes

**Design Status**: TBD (to be designed before PCB layout)

**Placement**: Top silkscreen, near USB connector or bottom corner

---

## 14. Matrix Scanning Frequency Analysis

### 14.1 Suggested Frequency: **1.5 kHz (0.67ms period)**

**Rationale (Heuristics and Engineering Considerations):**

#### Why 1-2 kHz Range?

**Human Perception Baseline:**
- Human typing speed: 60-120 WPM typical, 150+ WPM for fast typists
- Key press duration: 50-150ms typical
- Key travel time: 10-30ms (depends on switch)
- **Implication**: Scanning faster than ~10ms provides diminishing returns for detection, but helps with debouncing

**Debounce Requirements:**
- Mechanical switches bounce for 1-20ms (varies by switch, Matias typically 5-15ms)
- Need multiple consecutive stable readings to confirm state change
- With 5-10ms debounce target, need 5-10 scans at 1 kHz or 10-20 scans at 2 kHz
- **Sweet spot**: 1-2 kHz allows 5-20 samples during debounce period

**Latency Considerations:**
- 1 kHz (1ms) = ~1ms average latency (0.5-1.5ms range)
- 2 kHz (0.5ms) = ~0.5ms average latency (0.25-0.75ms range)
- **Human perception threshold**: ~10ms for input lag
- **Competitive gaming**: <5ms total latency desired
- **Conclusion**: Both 1 kHz and 2 kHz are well below perceptible latency

**Firmware Complexity:**
- 1 kHz (1ms): More time per scan for complex operations (encoder, macros, LED updates)
- 2 kHz (0.5ms): Tighter timing, more CPU cycles spent scanning
- **90-key matrix**: 10 rows scanned sequentially, ~100µs per row at 1 kHz = 10% CPU load
- **2 kHz would double CPU load**: ~200µs per scan = 40% CPU at 24 MHz

**USB Reporting:**
- USB Full-Speed: 1ms frame period (1 kHz polling)
- HID reports: Maximum 1 per 1ms frame (1 kHz effective limit)
- **Implication**: Scanning faster than 1 kHz doesn't improve USB report rate
- **But**: Faster scanning can improve debounce reliability and latency consistency

**PWM Considerations:**
- PWM frequency: 1-2 kHz suggested
- Matrix scan: 1-2 kHz
- **Potential aliasing if frequencies overlap**
- **Mitigation**: Slight offset (e.g., PWM at 1.8 kHz, scan at 1.5 kHz)

#### Why Not Faster (>2 kHz)?

- Diminishing returns on latency (already sub-millisecond)
- Increased CPU load (less time for other tasks)
- No USB reporting benefit (1 kHz USB limit)
- Potential EMI/noise concerns at higher frequencies
- Power consumption increases (irrelevant here, but general concern)

#### Why Not Slower (<1 kHz)?

- Higher average latency (>1ms)
- Fewer debounce samples (less robust)
- Approaching perceptible lag territory (<10ms budget)
- Less responsive feel

### 14.2 Final Recommendation: **1.5 kHz**

**Why 1.5 kHz specifically?**

1. **Middle ground**: Between 1 kHz and 2 kHz
2. **USB alignment**: 1.5× USB frame rate (good for averaging/filtering)
3. **PWM separation**: Easy to offset PWM (e.g., 1.8 kHz PWM, 1.5 kHz scan)
4. **Debounce sweet spot**: 7-15 samples in 5-10ms debounce window
5. **CPU load**: ~30% at 24 MHz (plenty of headroom for encoder, macros, LEDs)
6. **Latency**: ~0.67ms average (excellent responsiveness)

**Implementation:**
```c
#define MATRIX_SCAN_FREQUENCY_HZ 1500
#define MATRIX_SCAN_PERIOD_US (1000000 / MATRIX_SCAN_FREQUENCY_HZ)  // 667µs

// In main loop or timer ISR:
// Scan matrix every 667µs
```

**Validation on Dev Board:**
- Measure actual scan time (should be <400µs for 10 rows)
- Verify no aliasing with PWM (use oscilloscope if available)
- Test responsiveness with fast typing
- Tune debounce parameters (likely 5-10ms = 8-15 scans)

### 14.3 Alternative: Stay at 1 kHz or 2 kHz

**If 1.5 kHz complicates timer setup, use:**

- **1 kHz (1ms)**: Simpler math, USB-aligned, proven in QMK/other keyboards, ~33% CPU load
- **2 kHz (0.5ms)**: Lower latency, more debounce samples, ~40% CPU load

**Both are valid choices. 1.5 kHz is optimal, but 1 kHz or 2 kHz are perfectly acceptable if they simplify implementation.**

---

## 15. Bootloader Entry Sequence: Fn-Tap, Fn-Hold-LeftCtrl-RightCtrl

### 15.1 Updated Bootloader Entry Method

**Sequence**: 
1. **Tap Fn** (quick press and release)
2. **Hold Fn**
3. **Hold LeftCtrl** (while holding Fn)
4. **Hold RightCtrl** (while holding Fn and LeftCtrl)

**Rationale:**
- Requires deliberate action (less accidental triggering)
- Familiar pattern (Fn-based combo)
- Can be done one-handed on most keyboard layouts

### 15.2 Firmware Implementation

**State Machine Approach:**

```c
typedef enum {
    BOOTLOADER_STATE_IDLE,
    BOOTLOADER_STATE_FN_TAPPED,
    BOOTLOADER_STATE_FN_HELD,
    BOOTLOADER_STATE_FN_LCTRL,
    BOOTLOADER_STATE_FN_LCTRL_RCTRL
} bootloader_entry_state_t;

bootloader_entry_state_t bootloader_state = BOOTLOADER_STATE_IDLE;
uint32_t fn_tap_timestamp = 0;

#define FN_TAP_WINDOW_MS 500  // Time window after Fn tap to start hold sequence

void process_bootloader_entry(void) {
    uint32_t now = millis();
    
    switch (bootloader_state) {
        case BOOTLOADER_STATE_IDLE:
            if (fn_just_tapped()) {
                bootloader_state = BOOTLOADER_STATE_FN_TAPPED;
                fn_tap_timestamp = now;
            }
            break;
            
        case BOOTLOADER_STATE_FN_TAPPED:
            // Check if Fn tap window expired
            if ((now - fn_tap_timestamp) > FN_TAP_WINDOW_MS) {
                bootloader_state = BOOTLOADER_STATE_IDLE;
                break;
            }
            // Check if Fn is held
            if (fn_is_held()) {
                bootloader_state = BOOTLOADER_STATE_FN_HELD;
            }
            break;
            
        case BOOTLOADER_STATE_FN_HELD:
            if (!fn_is_held()) {
                // Fn released, abort
                bootloader_state = BOOTLOADER_STATE_IDLE;
                break;
            }
            // Check if LeftCtrl pressed while holding Fn
            if (key_is_pressed(KEY_LEFT_CTRL)) {
                bootloader_state = BOOTLOADER_STATE_FN_LCTRL;
            }
            break;
            
        case BOOTLOADER_STATE_FN_LCTRL:
            if (!fn_is_held() || !key_is_pressed(KEY_LEFT_CTRL)) {
                // Either released, abort
                bootloader_state = BOOTLOADER_STATE_IDLE;
                break;
            }
            // Check if RightCtrl pressed while holding Fn and LeftCtrl
            if (key_is_pressed(KEY_RIGHT_CTRL)) {
                bootloader_state = BOOTLOADER_STATE_FN_LCTRL_RCTRL;
                // SUCCESS - Enter bootloader
                request_bootloader();
            }
            break;
            
        case BOOTLOADER_STATE_FN_LCTRL_RCTRL:
            // Waiting for watchdog reset
            break;
    }
}

// Helper functions
bool fn_just_tapped(void) {
    // Detect Fn key tap (press and release within short time)
    // Implementation depends on layer handling
    static bool fn_was_pressed = false;
    bool fn_now = key_is_pressed(KEY_FN);
    bool tapped = (fn_was_pressed && !fn_now);
    fn_was_pressed = fn_now;
    return tapped;
}

bool fn_is_held(void) {
    // Check if Fn is currently held down
    return key_is_pressed(KEY_FN);
}
```

**Simpler Implementation (No State Machine):**

```c
// Check every scan cycle
void check_bootloader_entry(void) {
    static bool fn_tapped_recently = false;
    static uint32_t fn_tap_time = 0;
    uint32_t now = millis();
    
    // Detect Fn tap
    if (fn_just_tapped()) {
        fn_tapped_recently = true;
        fn_tap_time = now;
    }
    
    // Clear tap flag if window expired
    if (fn_tapped_recently && (now - fn_tap_time) > 500) {
        fn_tapped_recently = false;
    }
    
    // Check combo: Fn tapped recently AND Fn+LeftCtrl+RightCtrl all held
    if (fn_tapped_recently &&
        fn_is_held() &&
        key_is_pressed(KEY_LEFT_CTRL) &&
        key_is_pressed(KEY_RIGHT_CTRL)) {
        
        request_bootloader();  // Enter bootloader mode
    }
}
```

### 15.3 User Documentation

**docs/FLASHING.md:**

```markdown
## Entering Bootloader Mode

To update your AmberClick90 firmware:

1. **Tap Fn** (quick press and release)
2. **Hold Fn** (within 500ms of tap)
3. **Press and hold LeftCtrl** (while holding Fn)
4. **Press and hold RightCtrl** (while holding Fn and LeftCtrl)

The keyboard will disconnect and reconnect as "AmberClick90 Bootloader".

## Flashing Firmware

```bash
# Linux/macOS (LibUSB)
./amberclick-flash firmware.hex

# Windows (WinUSB)
amberclick-flash.exe firmware.hex
```

The keyboard will automatically restart after flashing.

## Troubleshooting

**Bootloader won't enter:**
- Ensure you tap Fn first, then hold it
- Complete the sequence within 500ms
- Make sure all three keys (Fn, LeftCtrl, RightCtrl) are held simultaneously

**Stuck in bootloader mode:**
- Unplug and replug USB cable
- Or flash valid firmware
- Or use UPDI recovery (see RECOVERY.md)
```

---

## 16. Outstanding Research Questions

### 16.1 Hardware Questions

1. **3D Printed Case**
   - What grey filament colors are available (PLA, PETG, ABS)?
   - Which grey shade best matches minimalist aesthetic?
   - Material choice for durability and printability

2. **Component Sourcing**
   - Matias Click/Quiet Click: current stock levels (90+ units needed)
   - Alps-compatible keycaps: availability, pricing, lead times
   - SOD-123 diodes: bulk pricing (100 units)

3. **Thermal Characterization**
   - AVR64DU32 temperature under continuous 24 MHz operation
   - Test on dev board with matrix scanning + USB + PWM
   - Determine if heatsinking needed (unlikely)

### 16.2 Firmware Questions

4. **GPR Preservation Across Reset Types**
   - Test on dev board: Does r2 survive RESET pin assertion?
   - Test on dev board: Does r2 survive direct software reset?
   - Document reliable reset mechanisms for bootloader entry

5. **USB Bootloader WinUSB Requirements**
   - Does Windows require driver installation for WinUSB?
   - Test on Windows 10/11 (may work driver-less)
   - Document installation process if driver needed

6. **MCC USB Stack Optimization**
   - Measure MCC-generated code size (HID and vendor protocol)
   - Trim unnecessary features for minimal footprint
   - Compare to LUFA implementations

7. **Debounce Tuning**
   - Test Matias Click bounce characteristics on real hardware
   - Determine optimal debounce time (likely 5-10ms)
   - Validate at 1.5 kHz scan rate (8-15 samples)

8. **PWM/Matrix Scan Aliasing**
   - Test PWM at 1.8 kHz, matrix at 1.5 kHz
   - Verify no visible artifacts or interference
   - Adjust frequencies if needed

### 16.3 Development Questions

9. **avrdude SerialUPDI Compatibility**
   - Verify avrdude version with AVR64DU32 support
   - Test CH340-based programmer with dev board
   - Document exact command-line syntax

10. **LibUSB Cross-Platform Testing**
    - Test vendor protocol bootloader on Linux
    - Test vendor protocol bootloader on macOS
    - Test vendor protocol bootloader on Windows (WinUSB)
    - Document platform-specific quirks

---

## 17. Document Status

This specification represents the **consolidated design decisions** as of this revision and serves as:

- Complete reference for schematic and PCB design
- Firmware architecture guide
- Component selection reference
- Development roadmap and timeline
- Testing checklist
- Handoff document for future development
- Comprehensive list of research questions to resolve

**Key Design Decisions:**
- Firmware targets AVR64DU32 specifically (best AVR-DU currently available)
- Moderate forward compatibility with future larger AVR-DU variants (via architecture)
- APPDATASIZE = 0x00 (all configuration compile-time, no VIA/VIAL)
- Bootloader entry: **Fn-tap, Fn-hold-LeftCtrl-RightCtrl** (deliberate sequence)
- Software reset on Layer 2 restarts firmware (does NOT enter bootloader)
- Encoder push switch is hardware reset only (not bootloader entry)
- Matias Click or Matias Quiet Click (identical footprints and plate requirements)
- SOD-123 diodes (hand-solderable with Hakko T18-C2 or T18-D12 tip)
- Leaded solder (63/37, 290-315°C)
- **6KRO** (not NKRO - simpler, sufficient, boot protocol compatible)
- **USB stack**: MCC-based (HID for keyboard, vendor protocol for bootloader)
- **NOT using TinyUSB** (using MCC + LUFA concepts + avrusbboot reference)
- **UPDI**: 3-pin header, CH340-based SerialUPDI programmer, avrdude
- **Matrix scan frequency**: **1.5 kHz recommended** (1 kHz or 2 kHz also acceptable)
- **Full-speed operation**: 24 MHz (no low-power modes, monitor thermal)
- **Ergonomic layout**: Already designed (90 keys, positions finalized)
- Copy-paste macro/encoder library (no preprocessor complexity, zero bloat)
- Users reflash for all configuration changes (traditional DIY keyboard workflow)
- **Scoped out**: USB MIDI, mouse emulation, on-the-fly macro recording (gimmicks)

**Next Steps:**
1. Acquire AVR64DU32 Nano dev board (mouser.ca) and CH340 SerialUPDI programmer
2. Test thermal behavior at 24 MHz under load
3. Test GPR preservation across reset types
4. Generate MCC USB projects (HID and vendor protocol)
5. Resolve outstanding research questions (Section 16)
6. Finalize typographic logo design
7. Begin firmware development on dev board
8. Create KiCad schematic (5 pages, ergonomic layout)
9. Complete PCB layout (2-layer, ergonomic switch positions)
10. Order PCBs, plate, and components
11. Assemble and test first prototype
12. Design and print 3D case (grey filament)
13. Document build process and publish

---

*Document Version: 4.0*  
*Last Updated: December 27, 2025*  
*Project: AmberClick90 (AVRDU.Enc.Mat)*



## Prompt:
12/28/2025, 9:25:44 AM

> GitHub: [JonathanFerron/MiyoCalc40](https://github.com/JonathanFerron/MiyoCalc40)

Note that i already have a ch340 based updi programmer. Make note to consider the simple debouncing approach used for MiyoCalc40 (see main.cop linked, lines 194 to 209). Also note that a PWM frequency of 368Hz is likely possible with the AVR-DU as it’s an option in DxCore for the AVR-DA. please produce a finak markdown artifact for this chat, that i will be able to hand over to a new chat to continue the work.



## Response:
12/28/2025, 9:27:41 AM

# AmberClick90 Project Handoff Document

## Project Summary

**AmberClick90** is a custom 90-key mechanical keyboard with the following characteristics:

- **MCU**: AVR64DU32 (best in AVR-DU family)
- **Layout**: 90 keys in ergonomic arrangement (already designed)
- **Switches**: Matias Click or Matias Quiet Click (interchangeable - identical footprints and plate requirements)
- **Encoder**: ALPS EC11 rotary encoder with integrated push switch (connected to RESET pin)
- **USB**: Full-Speed via USB Mini-B connector
- **LEDs**: 3× 3mm amber through-hole LEDs (1 power LED fixed brightness, 2 PWM indicators)
- **Programming**: UPDI (3-pin header) for initial/recovery, USB vendor protocol bootloader for updates
- **Power**: USB bus powered, hardwired to PC (110V outlet), full-speed 24MHz operation
- **Aesthetics**: Minimalist black PCB, grey 3D-printed case

---

## Key Technical Decisions

### Hardware

1. **MCU**: AVR64DU32-I/PT (TQFP-32)
   - 64 KB Flash, 8 KB SRAM, 24 MHz internal oscillator
   - 23 usable GPIO pins
   - Native USB Full-Speed (no external PHY)
   - UPDI programming interface
   - **Development**: AVR64DU32 Nano dev board (mouser.ca, affordable, quick delivery)

2. **Matrix**: 10 rows × 9 columns = 90 keys
   - **Diodes**: 1N4148W (SOD-123 package) - hand-solderable
   - **Scan frequency**: **1.5 kHz recommended** (1 kHz or 2 kHz acceptable)
   - **Debouncing**: Simple approach similar to MiyoCalc40 (see main.cpp lines 194-209)
   - **6KRO**: 6-key rollover + modifiers (not NKRO - simpler, sufficient, boot protocol compatible)

3. **Switches**: Matias Click OR Matias Quiet Click
   - Alps-compatible keycaps required
   - **Footprints and plate requirements identical between Click and Quiet variants**

4. **GPIO Allocation**:
   - **Columns (9)**: PA0–PA3, PD1–PD5
   - **Rows (10)**: PC3, PD0, PD6, PD7, PF0–PF5
   - **Encoder**: PA6 (A), PA7 (B)
   - **PWM LEDs**: PA4 (LED A), PA5 (LED B)
   - **RESET**: Encoder push switch

5. **USB Interface**:
   - Mini-B through-hole connector
   - D+/D- series resistors (22-33Ω)
   - Polyfuse (500mA) between VBUS and VDD
   - Optional: ESD protection diode array

6. **UPDI**: 3-pin header (GND, UPDI, VDD)
   - **Programmer**: CH340-based SerialUPDI (user already has one)
   - **Software**: avrdude with SerialUPDI support

7. **PWM Configuration**:
   - **Timer**: TCA (Timer Counter A)
   - **Frequency**: 1-2 kHz suggested (368 Hz also possible per DxCore)
   - **Avoid aliasing with matrix scan** (if matrix at 1.5 kHz, use PWM at ~1.8 kHz or ~368 Hz)
   - **Resolution**: 8-bit (0-255)

8. **LEDs**:
   - Power LED: ~6kΩ resistor (0.3-0.5mA, intentionally dim)
   - Indicator LEDs: 270Ω resistors (~5mA at 100%)
   - All 3mm through-hole, diffused amber

9. **Power**: Full-speed operation (24 MHz, no low-power modes)
   - Monitor thermal implications (test on dev board)
   - Hardwired to PC, no power constraints

### Firmware Architecture

1. **USB Stack**: MCC-based (NOT TinyUSB)
   - **Application (APPCODE)**: HID device (6KRO keyboard)
   - **Bootloader (BOOTCODE)**: Vendor protocol device (LibUSB/WinUSB)
   - Borrow concepts from LUFA and avrusbboot
   - Trim MCC-generated code for minimal footprint

2. **Memory Layout**:
   ```
   |------------------------| 0x0000
   | BOOTCODE (4-6 KB)      |
   | Vendor USB Bootloader  |
   | User determines size   |
   |------------------------| 0x1000-0x1800
   | APPCODE (58-60 KB)     |
   | Keyboard firmware      |
   | All keymaps/macros     |
   |------------------------| 0x10000
   | APPDATA (0 KB)         |
   | NOT USED               |
   |------------------------| 0x10000
   ```

3. **Fuse Configuration**:
   ```c
   BOOTSIZE = [User calculates]  // (bootloader_bytes / 512) rounded up
   APPDATASIZE = 0x00            // NOT USED
   UPDI: Enabled                 // NEVER DISABLE
   RSTPIN: Enabled               // Encoder switch
   ```

4. **Bootloader Entry**: **Fn-tap, Fn-hold-LeftCtrl-RightCtrl**
   - Tap Fn (quick press/release)
   - Hold Fn (within 500ms)
   - Hold LeftCtrl (while holding Fn)
   - Hold RightCtrl (while holding Fn and LeftCtrl)
   - GPR r2 stores magic value (0x42), watchdog reset triggers bootloader

5. **Layer System**:
   - **Layer 0**: Base (ergonomic layout)
   - **Layer 1**: Fn layer (function keys, media, macros)
   - **Layer 2**: System layer (LED brightness, bootloader entry, software reset)

6. **Data Storage**:
   - **Flash**: All keymaps, macros, USB descriptors, defaults (compile-time)
   - **SRAM**: Runtime state only (lost on power-off)
   - **No APPDATA section** - users reflash for configuration changes

7. **Debouncing**: Simple approach (MiyoCalc40 style)
   - See referenced main.cpp lines 194-209
   - 10 × 2ms = 20ms debounce window
   - Validate key press across multiple scans

8. **Toolchain**:
   - avr-gcc compiler
   - Geany text editor
   - avrdude with SerialUPDI support
   - MCC for USB stack generation
   - LibUSB/WinUSB for bootloader flashing tool

9. **Explicitly Scoped Out** (not implementing):
   - ❌ USB MIDI
   - ❌ Mouse emulation
   - ❌ On-the-fly macro recording
   - ❌ NKRO (using 6KRO instead)
   - ❌ VIA/VIAL runtime editing
   - ❌ APPDATA section

---

## Matrix Scanning Analysis

### Recommended: 1.5 kHz (0.67ms period)

**Rationale (Heuristics + Engineering)**:

1. **Human Perception**: Typing latency <10ms imperceptible; 1.5 kHz gives ~0.67ms average latency
2. **Debouncing**: With 5-10ms debounce target, 1.5 kHz provides 8-15 samples (good balance)
3. **USB Alignment**: 1.5× USB frame rate (1 kHz) - good for averaging/filtering
4. **CPU Load**: ~30% at 24 MHz (plenty of headroom for encoder, macros, LED updates)
5. **PWM Separation**: Easy to offset PWM (e.g., 1.8 kHz PWM, 1.5 kHz scan, or 368 Hz PWM)
6. **USB Reporting**: USB Full-Speed limited to 1 kHz polling anyway

**Alternatives**: 1 kHz or 2 kHz also acceptable if simplifies timer setup

**Implementation Note**: Matrix scan time should be <400µs for 10 rows at 24 MHz

---

## Bootloader Entry Implementation

### State Machine (Recommended)

```c
typedef enum {
    BOOTLOADER_STATE_IDLE,
    BOOTLOADER_STATE_FN_TAPPED,
    BOOTLOADER_STATE_FN_HELD,
    BOOTLOADER_STATE_FN_LCTRL,
    BOOTLOADER_STATE_FN_LCTRL_RCTRL
} bootloader_entry_state_t;

#define FN_TAP_WINDOW_MS 500

// Process each scan cycle
// Transitions: IDLE → FN_TAPPED (on Fn tap) → FN_HELD (on Fn hold)
// → FN_LCTRL (on LCtrl press) → FN_LCTRL_RCTRL (on RCtrl press)
// Success: request_bootloader() called
```

### Simpler Alternative

```c
// Track Fn tap within 500ms window
// Check: fn_tapped_recently && fn_held && lctrl_held && rctrl_held
// If all true: request_bootloader()
```

---

## Debouncing Reference (MiyoCalc40 Approach)

From provided main.cpp (lines 194-209):

```cpp
// Debounce 10 * 2ms = 20ms
press_valid = true;
for(uint8_t i = 0; i < 10; i++)
{
    // Initialize before scanning
    keypos_c = 0xff; keypos_r = 0xff;
    scanKB(); // Scan keyboard
    
    /* If no button is pressed, flag press as non-valid */
    if(keypos_c == 0xff && keypos_r == 0xff)
    {
        press_valid = false;
        break;
    }
    
    delay(2);  // Could use PIT-based delay for power savings
}

// If key press is valid after 10 consecutive scans, process it
if (press_valid) {
    // Process key press
}
```

**Key Points**:
- 10 consecutive scans with 2ms delay between each
- If any scan shows no key, abort (press_valid = false)
- Simple, effective, easy to understand
- Could optimize delay() to PIT-based for lower power (not critical for AmberClick90)

---

## Macro/Encoder Library System

**Repository Structure**:
```
/AmberClick90-firmware
├── src/                  # Active firmware (compiled)
│   ├── main.c
│   ├── keymap.c
│   ├── macros.c         # User's active macros
│   ├── encoder.c        # User's active encoder code
│   └── ...
├── lib/                  # Library (NOT compiled)
│   ├── macros/          # Copy-paste macro library
│   │   ├── README.md
│   │   ├── basic/
│   │   ├── media/
│   │   ├── window_management/
│   │   ├── development/
│   │   ├── productivity/
│   │   └── custom/
│   └── encoders/        # Copy-paste encoder library
│       ├── README.md
│       ├── basic_volume.c
│       ├── smart_alt_tab.c
│       └── template.c
└── docs/
    ├── BUILDING.md
    ├── FLASHING.md
    └── CUSTOMIZATION.md
```

**Workflow**:
1. User browses `lib/macros/` or `lib/encoders/`
2. User copies desired code to `src/macros.c` or `src/encoder.c`
3. Only copied code gets compiled (zero bloat)
4. No preprocessor complexity (`#ifdef` etc.)

**See separate macro/encoder library documentation for full details**

---

## Development Roadmap Summary

### Phase 1: Research & Setup (Weeks 1-2)
- Acquire AVR64DU32 Nano dev board (mouser.ca)
- CH340 SerialUPDI programmer (user already has)
- MCC USB stack research (HID + vendor protocol)
- avrusbboot reference study
- Test thermal behavior at 24 MHz
- Test GPR preservation across reset types

### Phase 2: Firmware on Dev Board (Weeks 2-6)
- Minimal HAL (GPIO, TCA PWM, watchdog)
- USB HID stack (6KRO)
- USB vendor protocol bootloader (LibUSB/WinUSB)
- Matrix scanning (simulated 3×3)
- Encoder emulation
- Debouncing (MiyoCalc40 style)
- Bootloader entry (Fn-tap-Fn-hold-Ctrl-Ctrl)

### Phase 3: Schematic (Weeks 3-4)
- 5 pages: Power, MCU, USB, Matrix, Peripherals
- Ergonomic layout (90 keys, already designed)
- 3-pin UPDI header
- Matias Click/Quiet Click footprints

### Phase 4: PCB Layout (Weeks 5-7)
- Import ergonomic layout
- USB differential pair routing (critical)
- SOD-123 diode placement
- Review and DRC

### Phase 5: Fabrication & Assembly (Weeks 8-10)
- Order PCBs (2-4 week lead time)
- Order components
- Assemble (diodes first, then MCU, passives, switches last)
- Test thermal under load

### Phase 6: Case & Finalization (Weeks 9-12)
- Design 3D case (grey filament)
- Test print and iterate
- Final assembly with keycaps
- Documentation

**Total Timeline**: 10-13 weeks

---

## Outstanding Research Questions

### Hardware
1. Grey filament options for 3D case (PLA, PETG, ABS)
2. Matias Click/Quiet Click stock levels (90+ units)
3. Alps-compatible keycap availability and pricing
4. Thermal characterization at 24 MHz continuous operation

### Firmware
5. GPR r2 preservation on RESET pin assertion (test on dev board)
6. GPR r2 preservation on direct software reset (test on dev board)
7. WinUSB driver requirements on Windows 10/11
8. MCC USB stack size optimization (trim unnecessary features)
9. Matias switch bounce characteristics (determine optimal debounce time)
10. PWM frequency selection to avoid aliasing with 1.5 kHz matrix scan

### Development
11. avrdude version with AVR64DU32 support verification
12. LibUSB cross-platform testing (Linux, macOS, Windows)

---

## Key References

### Datasheets
- [AVR64DU32 Datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/AVR64DU28-32-64-DataSheet-DS40002576.pdf)
- [ALPS EC11 Encoder](https://tech.alpsalpine.com/e/products/detail/EC11E15244G1/)
- [Matias Switches](https://matias.ca/switches/datasheet.pdf)
- [1N4148W Diode](https://www.diodes.com/assets/Datasheets/ds30086.pdf)

### Software/Tools
- Microchip Code Configurator (MCC) - USB stack
- avrusbboot - vendor protocol bootloader reference
- LUFA - USB concepts
- avr-gcc, avrdude, SerialUPDI
- LibUSB, WinUSB

### Hardware
- AVR64DU32 Nano dev board (mouser.ca)
- CH340-based SerialUPDI programmer (user has)

### Example Code
- MiyoCalc40 firmware (main.cpp) - debouncing approach, matrix scanning, sleep mode

---

## Component BOM Summary

| Component | Specification | Qty | Notes |
|-----------|---------------|-----|-------|
| MCU | AVR64DU32-I/PT | 1 | TQFP-32 |
| Dev Board | AVR64DU32 Nano | 1 | mouser.ca |
| UPDI Programmer | CH340-based | 1 | User has |
| USB Connector | Mini-B TH | 1 | |
| UPDI Header | 3-pin, 0.1" | 1 | GND, UPDI, VDD |
| Switches | Matias Click/Quiet | 90 | Interchangeable |
| Keycaps | Alps-compatible | 90+ | |
| Diodes | 1N4148W SOD-123 | 100 | Hand-solderable |
| Encoder | ALPS EC11 | 1 | With switch |
| LEDs | 3mm amber TH diffused | 3 | 1 power, 2 PWM |
| Resistors | 270Ω (LED), ~6kΩ (power LED) | 3 | 0805 or TH |
| USB Resistors | 22-33Ω, 0805 | 2 | D+, D- |
| Decoupling Caps | 100nF, 0805 | 6-9 | VDD, VUSB |
| Bulk Cap | 4.7-10µF | 1 | Near USB |
| Polyfuse | 500mA | 1 | VBUS to VDD |

---

## Design Philosophy

1. **Simplicity**: Direct register access, minimal abstraction, clean code
2. **Compile-time configuration**: No runtime editing, users reflash for changes
3. **Zero bloat**: Copy-paste macro library, only used code compiled
4. **Traditional DIY workflow**: Git-friendly, version-controlled configurations
5. **6KRO not NKRO**: Simpler, sufficient for >99% use cases
6. **Full-speed operation**: No power constraints, monitor thermal instead
7. **Ergonomic layout**: Already designed, 90 keys optimized for typing

---

## Critical Reminders

1. **NEVER disable UPDI via fuses** (required for recovery)
2. **User determines BOOTSIZE** after bootloader compilation: `(bytes / 512) rounded up`
3. **APPDATASIZE = 0x00** (not using APPDATA section)
4. **Matias Click and Quiet Click are interchangeable** (same footprint, same plate)
5. **SOD-123 diodes**: Hand-solderable with Hakko T18-C2 or T18-D12 tip, 290-315°C
6. **Matrix scan at 1.5 kHz**, PWM at different frequency to avoid aliasing
7. **Debouncing**: Use MiyoCalc40 approach (10 × 2ms validation)
8. **Bootloader entry**: Fn-tap, then Fn-hold-LeftCtrl-RightCtrl within 500ms
9. **3-pin UPDI header**: GND, UPDI, VDD (verify from datasheet)
10. **CH340-based SerialUPDI works with avrdude** (user already has programmer)

---

## Next Chat Starting Points

1. **Immediate**: Test GPR preservation on dev board with various reset types
2. **Firmware**: Begin MCC USB stack generation and trimming
3. **Bootloader**: Study avrusbboot vendor protocol implementation
4. **Schematic**: Start KiCad schematic (5 pages) with ergonomic layout
5. **Thermal**: Characterize AVR64DU32 at 24 MHz under load
6. **PCB**: Plan component placement for ergonomic layout

---

*Document Version: Final Handoff v1.0*  
*Date: December 27, 2025*  
*Project: AmberClick90 (AVRDU.Enc.Mat)*



## Prompt:
12/30/2025, 6:42:47 PM

create a high level tree with folder names, file names (along with short description of their purpose in 2 to 5 words), suggested for the project under git.



## Response:
12/30/2025, 6:43:42 PM

# AmberClick90 Git Repository Structure

```
AmberClick90/
│
├── README.md                           # Project overview, quick start
├── LICENSE                             # Open source license
├── .gitignore                          # Ignore build artifacts, hex files
│
├── docs/                               # Documentation
│   ├── README.md                       # Documentation index
│   ├── TECHNICAL_SPEC.md               # Complete technical specification
│   ├── BUILDING.md                     # Firmware build instructions
│   ├── FLASHING.md                     # Bootloader entry, flashing guide
│   ├── CUSTOMIZATION.md                # How to customize keymaps
│   ├── API.md                          # Keyboard firmware API reference
│   ├── RECOVERY.md                     # UPDI unbrick procedure
│   ├── BOM.md                          # Bill of materials
│   ├── ASSEMBLY.md                     # PCB assembly instructions
│   └── images/                         # Photos, diagrams
│       ├── pcb_layout.png
│       ├── assembled.jpg
│       └── schematic_overview.png
│
├── hardware/                           # PCB design files
│   ├── README.md                       # Hardware documentation
│   ├── kicad/                          # KiCad project
│   │   ├── AmberClick90.kicad_pro      # KiCad project file
│   │   ├── AmberClick90.kicad_sch      # Main schematic
│   │   ├── AmberClick90.kicad_pcb      # PCB layout
│   │   ├── AmberClick90-cache.lib      # Component cache
│   │   ├── sym-lib-table               # Symbol library table
│   │   ├── fp-lib-table                # Footprint library table
│   │   ├── gerbers/                    # Gerber files for fabrication
│   │   │   ├── AmberClick90-F_Cu.gbr
│   │   │   ├── AmberClick90-B_Cu.gbr
│   │   │   └── ...
│   │   └── 3d_models/                  # 3D models for PCB
│   │       └── AVR64DU32.step
│   ├── schematic.pdf                   # PDF export of schematic
│   ├── bom.csv                         # Bill of materials export
│   └── datasheets/                     # Component datasheets
│       ├── AVR64DU32.pdf
│       ├── ALPS_EC11.pdf
│       ├── Matias_switches.pdf
│       └── 1N4148W.pdf
│
├── case/                               # 3D printed case
│   ├── README.md                       # Case assembly instructions
│   ├── stl/                            # STL files for printing
│   │   ├── top_case.stl
│   │   ├── bottom_case.stl
│   │   ├── plate.stl
│   │   └── encoder_knob.stl
│   ├── cad/                            # CAD source files
│   │   ├── AmberClick90_case.f3d       # Fusion 360 source
│   │   └── AmberClick90_case.step      # STEP export
│   └── printing_notes.md               # Filament, print settings notes
│
├── firmware/                           # Keyboard firmware
│   ├── README.md                       # Firmware overview
│   ├── Makefile                        # Build system
│   ├── config.mk                       # Build configuration
│   │
│   ├── src/                            # Active source code
│   │   ├── main.c                      # Entry point, main loop
│   │   ├── config.h                    # Firmware configuration constants
│   │   ├── keymap.c                    # User keymap definitions
│   │   ├── macros.c                    # User active macros
│   │   ├── encoder.c                   # Encoder behavior implementation
│   │   ├── matrix.c                    # Matrix scanning logic
│   │   ├── matrix.h
│   │   ├── debounce.c                  # Debouncing algorithm
│   │   ├── debounce.h
│   │   ├── layers.c                    # Layer switching logic
│   │   ├── layers.h
│   │   ├── keyboard.c                  # Keyboard state management
│   │   ├── keyboard.h
│   │   ├── usb_hid.c                   # USB HID keyboard implementation
│   │   ├── usb_hid.h
│   │   ├── bootloader.c                # Bootloader entry logic
│   │   ├── bootloader.h
│   │   ├── hal_gpio.c                  # GPIO hardware abstraction
│   │   ├── hal_gpio.h
│   │   ├── hal_timer.c                 # Timer/PWM abstraction
│   │   ├── hal_timer.h
│   │   ├── led.c                       # LED control, PWM
│   │   ├── led.h
│   │   └── util.c                      # Utility functions
│   │       └── util.h
│   │
│   ├── mcc_generated/                  # MCC USB stack
│   │   ├── usb/                        # USB stack (trimmed)
│   │   │   ├── usb_device.c
│   │   │   ├── usb_device.h
│   │   │   ├── usb_hid.c
│   │   │   └── usb_hid.h
│   │   └── system/                     # System initialization
│   │       ├── clock.c
│   │       └── pins.c
│   │
│   ├── bootloader/                     # Bootloader firmware
│   │   ├── README.md                   # Bootloader documentation
│   │   ├── Makefile                    # Bootloader build
│   │   ├── main.c                      # Bootloader entry point
│   │   ├── usb_vendor.c                # USB vendor protocol
│   │   ├── usb_vendor.h
│   │   ├── flash.c                     # Flash write routines
│   │   ├── flash.h
│   │   └── linker.ld                   # Bootloader linker script
│   │
│   ├── lib/                            # Macro/encoder library (NOT compiled)
│   │   ├── README.md                   # Library usage guide
│   │   │
│   │   ├── macros/                     # Macro library
│   │   │   ├── README.md               # Macro library index
│   │   │   ├── basic/
│   │   │   │   ├── copy_paste.c        # Ctrl+C, Ctrl+V, Ctrl+X
│   │   │   │   ├── text_editing.c      # Line operations, word nav
│   │   │   │   └── navigation.c        # Home/End combos
│   │   │   ├── media/
│   │   │   │   ├── playback.c          # Play/pause, next/prev
│   │   │   │   └── volume.c            # Volume up/down, mute
│   │   │   ├── window_management/
│   │   │   │   ├── alt_tab.c           # Alt-Tab implementations
│   │   │   │   ├── virtual_desktops.c  # Desktop switching
│   │   │   │   └── tiling.c            # Window snapping
│   │   │   ├── development/
│   │   │   │   ├── ide_shortcuts.c     # VSCode, IntelliJ shortcuts
│   │   │   │   ├── terminal.c          # Terminal commands
│   │   │   │   └── git.c               # Git shortcuts
│   │   │   ├── productivity/
│   │   │   │   ├── email.c             # Email signatures
│   │   │   │   ├── zoom.c              # Zoom meeting shortcuts
│   │   │   │   └── browser.c           # Tab management
│   │   │   └── custom/
│   │   │       └── template.c          # Empty macro template
│   │   │
│   │   ├── encoders/                   # Encoder library
│   │   │   ├── README.md               # Encoder library index
│   │   │   ├── basic_volume.c          # Simple volume control
│   │   │   ├── smart_alt_tab.c         # Alt-Tab with hold
│   │   │   ├── scroll_wheel.c          # Mouse wheel emulation
│   │   │   ├── zoom_control.c          # Browser zoom
│   │   │   ├── brightness.c            # Screen brightness
│   │   │   ├── timeline.c              # Media scrubbing
│   │   │   └── template.c              # Empty encoder template
│   │   │
│   │   └── keymaps/                    # Example keymaps
│   │       ├── default/
│   │       │   ├── keymap.c            # Default layout
│   │       │   └── README.md           # Layout description
│   │       ├── programmer/
│   │       │   ├── keymap.c            # Programmer-optimized layout
│   │       │   └── README.md
│   │       ├── gaming/
│   │       │   ├── keymap.c            # Gaming layout
│   │       │   └── README.md
│   │       └── vim_user/
│   │           ├── keymap.c            # Vim-optimized layout
│   │           └── README.md
│   │
│   └── build/                          # Build output (gitignored)
│       ├── AmberClick90.hex
│       ├── AmberClick90.elf
│       └── *.o
│
├── tools/                              # Development tools
│   ├── README.md                       # Tools documentation
│   ├── flash/                          # Flashing tools
│   │   ├── amberclick-flash            # USB bootloader flasher (Linux/macOS)
│   │   ├── amberclick-flash.exe        # USB bootloader flasher (Windows)
│   │   ├── flash.c                     # Flasher source (LibUSB)
│   │   ├── flash.h
│   │   ├── Makefile                    # Flasher build
│   │   └── README.md                   # Flashing tool usage
│   ├── updi/                           # UPDI programming
│   │   ├── flash_bootloader.sh         # Script to flash bootloader
│   │   ├── flash_firmware.sh           # Script to flash firmware
│   │   └── README.md                   # UPDI programming guide
│   └── test/                           # Test utilities
│       ├── key_test.c                  # Key tester firmware
│       └── README.md                   # Testing documentation
│
├── tests/                              # Firmware tests
│   ├── test_matrix.c                   # Matrix scanning tests
│   ├── test_debounce.c                 # Debounce tests
│   ├── test_encoder.c                  # Encoder tests
│   └── README.md                       # Testing guide
│
└── examples/                           # Example configurations
    ├── README.md                       # Examples documentation
    ├── my_layout/                      # Example: Personal layout
    │   ├── keymap.c
    │   ├── macros.c
    │   └── README.md
    └── office_productivity/            # Example: Office layout
        ├── keymap.c
        ├── macros.c
        └── README.md
```

## Key Notes

### Compiled vs Not Compiled
- **Compiled**: `firmware/src/`, `firmware/bootloader/`, `firmware/mcc_generated/`
- **NOT Compiled**: `firmware/lib/` (reference library for copy-paste)
- **Build Output**: `firmware/build/` (gitignored)

### User-Modifiable Files
- `firmware/src/keymap.c` - User's active keymap
- `firmware/src/macros.c` - User's active macros (copied from lib)
- `firmware/src/encoder.c` - User's encoder behavior (copied from lib)
- `firmware/src/config.h` - Firmware configuration (brightness, debounce, etc.)

### Reference Files (Copy From Here)
- `firmware/lib/macros/*` - Copy functions to `src/macros.c`
- `firmware/lib/encoders/*` - Copy code to `src/encoder.c`
- `firmware/lib/keymaps/*` - Reference complete keymaps

### gitignore Suggestions
```gitignore
# Build artifacts
firmware/build/
*.o
*.elf
*.hex
*.map
*.lst

# KiCad backups
hardware/kicad/*-backups/
hardware/kicad/*-cache.lib
hardware/kicad/fp-info-cache

# Editor files
*.swp
*~
.vscode/
.DS_Store

# User-specific configs (optional)
# firmware/src/keymap.c
# firmware/src/macros.c
```

This structure provides clear separation between:
- **Hardware** (PCB, case, datasheets)
- **Firmware** (source, bootloader, library, tools)
- **Documentation** (guides, specs, assembly)
- **Examples** (reference configurations)



## Prompt:
12/30/2025, 10:59:21 PM

by way of a few short examples, please explain what you consider is a light touch of harware abstraction layer (hal) as suggested earlier in this chat for gpio, timer (leds pwm), etc.



## Response:
12/30/2025, 11:00:28 PM

# Light-Touch HAL Examples for AmberClick90

## Philosophy

A **light-touch HAL** provides just enough abstraction to:
1. Make code readable (no magic register addresses)
2. Keep hardware details in one place (easier porting)
3. Add zero runtime overhead (inline functions, macros)
4. Stay close to hardware (direct register access visible)

**NOT a heavy HAL**: No complex state machines, no callbacks, no "framework" - just thin wrappers.

---

## Example 1: GPIO HAL (Minimal)

### hal_gpio.h
```c
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <avr/io.h>
#include <stdbool.h>

// Pin definitions (already in your design)
#define ROW_0_PIN   PIN3_bm
#define ROW_0_PORT  PORTC
#define ROW_0_VPORT VPORTC

#define COL_0_PIN   PIN0_bm
#define COL_0_PORT  PORTC
#define COL_0_VPORT VPORTC

// ... (define all 23 GPIO pins similarly)

// GPIO direction
static inline void gpio_set_output(PORT_t *port, uint8_t pin_bm) {
    port->DIRSET = pin_bm;
}

static inline void gpio_set_input(PORT_t *port, uint8_t pin_bm) {
    port->DIRCLR = pin_bm;
}

// GPIO write (use VPORT for speed)
static inline void gpio_write_high(VPORT_t *vport, uint8_t pin_bm) {
    vport->OUT |= pin_bm;
}

static inline void gpio_write_low(VPORT_t *vport, uint8_t pin_bm) {
    vport->OUT &= ~pin_bm;
}

static inline void gpio_toggle(VPORT_t *vport, uint8_t pin_bm) {
    vport->IN = pin_bm;  // Write to IN register toggles
}

// GPIO read (use VPORT for speed)
static inline bool gpio_read(VPORT_t *vport, uint8_t pin_bm) {
    return (vport->IN & pin_bm) != 0;
}

// Pull-up configuration
static inline void gpio_set_pullup(PORT_t *port, uint8_t pin_num) {
    // Use register_t array access: PIN0CTRL, PIN1CTRL, etc.
    *((uint8_t *)&port->PIN0CTRL + pin_num) |= PORT_PULLUPEN_bm;
}

#endif // HAL_GPIO_H
```

### Usage in matrix.c
```c
#include "hal_gpio.h"

void matrix_init(void) {
    // Set rows as outputs (driven)
    gpio_set_output(&ROW_0_PORT, ROW_0_PIN);
    gpio_set_output(&ROW_1_PORT, ROW_1_PIN);
    // ... repeat for all rows
    
    // Set columns as inputs with pull-ups
    gpio_set_input(&COL_0_PORT, COL_0_PIN);
    gpio_set_pullup(&COL_0_PORT, 0);  // PIN0
    // ... repeat for all columns
}

void matrix_scan_row(uint8_t row) {
    // Drive row low
    switch(row) {
        case 0: gpio_write_low(&ROW_0_VPORT, ROW_0_PIN); break;
        case 1: gpio_write_low(&ROW_1_VPORT, ROW_1_PIN); break;
        // ...
    }
    
    // Read columns (pull-ups mean pressed = LOW)
    bool col0_pressed = !gpio_read(&COL_0_VPORT, COL_0_PIN);
    bool col1_pressed = !gpio_read(&COL_1_VPORT, COL_1_PIN);
    // ...
    
    // Return row to high
    switch(row) {
        case 0: gpio_write_high(&ROW_0_VPORT, ROW_0_PIN); break;
        // ...
    }
}
```

**What makes this "light"**:
- `static inline` = zero overhead (compiler inlines)
- Direct register access still visible (`port->DIRSET`)
- No malloc, no state tracking, no initialization functions
- Just readable names for common operations

---

## Example 2: Timer HAL (PWM for LEDs)

### hal_timer.h
```c
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <avr/io.h>

// TCA0 PWM channels (WO0-WO5 available, we use WO1, WO4, WO5)
// PA1 = WO1 (backlight - not in current design but shown as example)
// PA4 = WO4 (LED A)
// PA5 = WO5 (LED B)

// Initialize TCA0 for 8-bit split mode PWM
static inline void pwm_init(void) {
    // Route TCA0 to PORTA
    PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;
    
    // Enable split mode (gives us 6 independent 8-bit PWM channels)
    TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;
    
    // Set period (TOP value) for ~1.5 kHz
    // f_PWM = F_CPU / (prescaler * (PER + 1))
    // 1500 Hz = 24 MHz / (64 * (PER + 1))
    // PER = 249
    TCA0.SPLIT.HPER = 249;  // High byte period
    TCA0.SPLIT.LPER = 249;  // Low byte period
    
    // Prescaler DIV64
    TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm;
}

// Set PWM duty cycle (0-255)
static inline void pwm_set_duty(uint8_t channel, uint8_t duty) {
    switch(channel) {
        case 1: TCA0.SPLIT.LCMP1 = duty; break;  // WO1 (PA1)
        case 4: TCA0.SPLIT.HCMP0 = duty; break;  // WO4 (PA4)
        case 5: TCA0.SPLIT.HCMP1 = duty; break;  // WO5 (PA5)
    }
}

// Enable PWM output on specific pin
static inline void pwm_enable_output(uint8_t channel) {
    switch(channel) {
        case 1: TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP1EN_bm; break;
        case 4: TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP0EN_bm; break;
        case 5: TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP1EN_bm; break;
    }
}

// Disable PWM output
static inline void pwm_disable_output(uint8_t channel) {
    switch(channel) {
        case 1: TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_LCMP1EN_bm; break;
        case 4: TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_HCMP0EN_bm; break;
        case 5: TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_HCMP1EN_bm; break;
    }
}

#endif // HAL_TIMER_H
```

### Usage in led.c
```c
#include "hal_timer.h"
#include "hal_gpio.h"

#define LED_A_CHANNEL 4  // PA4 = WO4
#define LED_B_CHANNEL 5  // PA5 = WO5

void led_init(void) {
    // Set LED pins as outputs
    gpio_set_output(&PORTA, PIN4_bm);  // LED A
    gpio_set_output(&PORTA, PIN5_bm);  // LED B
    
    // Initialize PWM
    pwm_init();
    
    // Enable PWM outputs
    pwm_enable_output(LED_A_CHANNEL);
    pwm_enable_output(LED_B_CHANNEL);
    
    // Set default brightness (50%)
    pwm_set_duty(LED_A_CHANNEL, 128);
    pwm_set_duty(LED_B_CHANNEL, 128);
}

void led_set_brightness(uint8_t led_id, uint8_t brightness) {
    if (led_id == 0) {
        pwm_set_duty(LED_A_CHANNEL, brightness);
    } else if (led_id == 1) {
        pwm_set_duty(LED_B_CHANNEL, brightness);
    }
}
```

**What makes this "light"**:
- One-time initialization (no state tracking)
- Direct register writes for duty cycle
- No callbacks, no interrupts (unless you add them)
- Readable but still shows what's happening (`TCA0.SPLIT.HCMP0`)

---

## Example 3: Debounce (Not Really HAL, but Shows Approach)

### debounce.c (Based on MiyoCalc40)
```c
#include "debounce.h"
#include "matrix.h"
#include <util/delay.h>

#define DEBOUNCE_SAMPLES 10
#define DEBOUNCE_DELAY_MS 2

bool debounce_key_press(void) {
    bool press_valid = true;
    
    for (uint8_t i = 0; i < DEBOUNCE_SAMPLES; i++) {
        // Scan matrix
        uint8_t row, col;
        matrix_scan(&row, &col);
        
        // If no key detected, press is invalid
        if (row == 0xFF && col == 0xFF) {
            press_valid = false;
            break;
        }
        
        _delay_ms(DEBOUNCE_DELAY_MS);
    }
    
    return press_valid;
}
```

**What makes this "light"**:
- Simple loop, no state machine
- Uses `_delay_ms()` for now (can optimize to PIT later)
- Easy to understand and modify
- No complex data structures

---

## Example 4: Ultra-Light ADC (Battery Monitoring - Not in Current Design)

### hal_adc.h (If you ever need it)
```c
#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <avr/io.h>

// Initialize ADC for VDD measurement
static inline void adc_init_vdd(void) {
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;
    VREF.ACREF = VREF_REFSEL_VDD_gc;
    AC0.DACREF = 128;
    ADC0.MUXPOS = ADC_MUXPOS_DACREF0_gc;
    ADC0.CTRLC = ADC_PRESC_DIV64_gc;
    ADC0.CTRLA = ADC_ENABLE_bm;
}

// Start conversion and wait for result
static inline uint16_t adc_read(void) {
    ADC0.COMMAND = ADC_STCONV_bm;
    while (ADC0.COMMAND & ADC_STCONV_bm);
    return ADC0.RES;
}

// Disable ADC to save power
static inline void adc_disable(void) {
    ADC0.CTRLA &= ~ADC_ENABLE_bm;
}

#endif // HAL_ADC_H
```

**What makes this "light"**:
- Three simple functions, all inline
- No initialization routine with callbacks
- Enable, read, disable - that's it
- Easy to see what registers are being touched

---

## Contrast: What a "Heavy" HAL Would Look Like (DON'T DO THIS)

```c
// HEAVY HAL - TOO MUCH ABSTRACTION
typedef struct {
    void (*write_callback)(uint8_t value);
    uint8_t state;
    bool initialized;
    uint8_t buffer[16];
} GPIO_Handle_t;

HAL_StatusTypeDef HAL_GPIO_Init(GPIO_Handle_t *handle, GPIO_Config_t *config);
HAL_StatusTypeDef HAL_GPIO_Write(GPIO_Handle_t *handle, uint8_t value);
// ... tons of functions, state tracking, error codes, etc.
```

**Problems with heavy HAL**:
- Runtime overhead (function calls, state checks)
- Memory overhead (state structures)
- Complexity (hard to debug, follow code flow)
- Abstracts away too much (harder to optimize)

---

## Summary: Light HAL Guidelines

1. **Use `static inline`** for everything possible (zero overhead)
2. **No heap allocation** (no malloc, no state structures)
3. **Direct register access still visible** in function bodies
4. **Minimal API surface** (only wrap common operations)
5. **No callbacks** unless absolutely necessary
6. **No error codes** for simple operations (use assertions/debug only)
7. **Keep it in headers** (encourages inlining)
8. **Name things clearly** but don't hide the hardware

### File Organization

```
firmware/src/
├── hal_gpio.h       # GPIO macros and inline functions
├── hal_timer.h      # TCA PWM setup and control
├── hal_adc.h        # ADC if needed (optional)
└── (no .c files)    # All inline, no separate compilation
```

### When to Add HAL

✅ **Good reasons**:
- Multiple similar operations (setting pin directions)
- Register names are cryptic (`TCA0.SPLIT.HCMP0`)
- Same operation on different pins/peripherals
- Porting consideration (isolate hardware details)

❌ **Bad reasons**:
- "Professional code should be abstracted"
- "We might change hardware later" (YAGNI)
- "Need to hide complexity" (don't hide, simplify)
- "Other projects do it this way"

### Result

Your firmware stays:
- **Fast** (inline functions = no overhead)
- **Small** (no abstraction bloat)
- **Readable** (`gpio_write_high()` vs `VPORTC.OUT |= PIN3_bm`)
- **Debuggable** (can still see registers in debugger)
- **Portable** (hardware details isolated, but not hidden)

This is the spirit of embedded "bare-metal" programming: thin wrappers that make code readable without sacrificing performance or transparency.



## Prompt:
12/30/2025, 11:17:19 PM

For matrix scanning, there are some operations that will need to be applied to several row or column pins in the same port (as i tried to group rows in as few ports as possible, and same for column pins). What are a couple of light HAL that I could consider adopting to optimize the code a bit by applying operations (input vs output, high vs low, input pull-up, etc) at the port or vport level via a bitmask to select the affected pins?



## Response:
12/30/2025, 11:18:36 PM

# Port-Level HAL for Matrix Scanning Optimization

## Key Insight

When multiple pins are on the same port, you can operate on them **simultaneously with a single register write** using bitmasks. This is much faster than individual pin operations.

---

## Your Pin Groupings (From Spec)

```c
// Columns (9 pins across 3 ports)
// PORTC: PA0, PA1, PA2, PA3         (4 pins)
// PORTD: PD1, PD2, PD3, PD4, PD5    (5 pins)

// Rows (10 pins across 3 ports)  
// PORTC: PC3                        (1 pin)
// PORTD: PD0, PD6, PD7              (3 pins)
// PORTF: PF0, PF1, PF2, PF3, PF4, PF5  (6 pins)
```

**Opportunity**: PF0-PF5 (6 rows) can be set/read together!

---

## Example 1: Port-Level GPIO HAL

### hal_gpio.h (Port Operations)

```c
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <avr/io.h>
#include <stdbool.h>

//=============================================================================
// SINGLE PIN OPERATIONS (Keep these from before)
//=============================================================================

static inline void gpio_set_output(PORT_t *port, uint8_t pin_bm) {
    port->DIRSET = pin_bm;
}

static inline void gpio_set_input(PORT_t *port, uint8_t pin_bm) {
    port->DIRCLR = pin_bm;
}

static inline void gpio_write_high(VPORT_t *vport, uint8_t pin_bm) {
    vport->OUT |= pin_bm;
}

static inline void gpio_write_low(VPORT_t *vport, uint8_t pin_bm) {
    vport->OUT &= ~pin_bm;
}

static inline bool gpio_read(VPORT_t *vport, uint8_t pin_bm) {
    return (vport->IN & pin_bm) != 0;
}

//=============================================================================
// PORT-LEVEL OPERATIONS (Multiple pins simultaneously)
//=============================================================================

// Set multiple pins as outputs at once
static inline void gpio_set_output_mask(PORT_t *port, uint8_t pin_mask) {
    port->DIRSET = pin_mask;
}

// Set multiple pins as inputs at once
static inline void gpio_set_input_mask(PORT_t *port, uint8_t pin_mask) {
    port->DIRCLR = pin_mask;
}

// Write multiple pins high at once
static inline void gpio_write_high_mask(VPORT_t *vport, uint8_t pin_mask) {
    vport->OUT |= pin_mask;
}

// Write multiple pins low at once
static inline void gpio_write_low_mask(VPORT_t *vport, uint8_t pin_mask) {
    vport->OUT &= ~pin_mask;
}

// Set specific pins (mask) to specific values (value)
// Pins not in mask are unchanged
static inline void gpio_write_mask(VPORT_t *vport, uint8_t pin_mask, uint8_t value) {
    uint8_t current = vport->OUT;
    current &= ~pin_mask;        // Clear bits in mask
    current |= (value & pin_mask); // Set bits from value
    vport->OUT = current;
}

// Read multiple pins at once, return masked result
static inline uint8_t gpio_read_mask(VPORT_t *vport, uint8_t pin_mask) {
    return vport->IN & pin_mask;
}

// Configure pull-ups for multiple pins
// pin_mask: which pins to configure (1 = configure)
static inline void gpio_set_pullup_mask(PORT_t *port, uint8_t pin_mask) {
    // Unfortunately, PINnCTRL registers aren't contiguous in a way
    // that allows easy masking. Need to loop through bits.
    for (uint8_t i = 0; i < 8; i++) {
        if (pin_mask & (1 << i)) {
            *((uint8_t *)&port->PIN0CTRL + i) |= PORT_PULLUPEN_bm;
        }
    }
}

// Faster version: configure pull-ups for contiguous pins
// Example: pins 0-5 on PORTF
static inline void gpio_set_pullup_range(PORT_t *port, uint8_t first_pin, uint8_t last_pin) {
    for (uint8_t i = first_pin; i <= last_pin; i++) {
        *((uint8_t *)&port->PIN0CTRL + i) |= PORT_PULLUPEN_bm;
    }
}

#endif // HAL_GPIO_H
```

---

## Example 2: Matrix-Specific HAL

### matrix.h (Define Port Groupings)

```c
#ifndef MATRIX_H
#define MATRIX_H

#include <avr/io.h>

//=============================================================================
// COLUMN PIN DEFINITIONS (Inputs with pull-ups)
//=============================================================================

// PORTA columns: PA0, PA1, PA2, PA3 (4 pins)
#define COL_PORTA_MASK  0b00001111  // Pins 0-3
#define COL_PORTA_PORT  PORTA
#define COL_PORTA_VPORT VPORTA

// PORTD columns: PD1, PD2, PD3, PD4, PD5 (5 pins)
#define COL_PORTD_MASK  0b00111110  // Pins 1-5
#define COL_PORTD_PORT  PORTD
#define COL_PORTD_VPORT VPORTD

//=============================================================================
// ROW PIN DEFINITIONS (Outputs, normally high)
//=============================================================================

// PORTC rows: PC3 (1 pin)
#define ROW_PORTC_MASK  0b00001000  // Pin 3
#define ROW_PORTC_PORT  PORTC
#define ROW_PORTC_VPORT VPORTC

// PORTD rows: PD0, PD6, PD7 (3 pins)
#define ROW_PORTD_MASK  0b11000001  // Pins 0, 6, 7
#define ROW_PORTD_PORT  PORTD
#define ROW_PORTD_VPORT VPORTD

// PORTF rows: PF0, PF1, PF2, PF3, PF4, PF5 (6 pins) ← BEST GROUPING
#define ROW_PORTF_MASK  0b00111111  // Pins 0-5
#define ROW_PORTF_PORT  PORTF
#define ROW_PORTF_VPORT VPORTF

//=============================================================================
// INDIVIDUAL ROW PINS (for sequential scanning)
//=============================================================================

#define ROW_0_BM  PIN3_bm   // PC3
#define ROW_0_VPORT VPORTC

#define ROW_1_BM  PIN0_bm   // PD0
#define ROW_1_VPORT VPORTD

#define ROW_2_BM  PIN6_bm   // PD6
#define ROW_2_VPORT VPORTD

#define ROW_3_BM  PIN7_bm   // PD7
#define ROW_3_VPORT VPORTD

#define ROW_4_BM  PIN0_bm   // PF0
#define ROW_4_VPORT VPORTF

#define ROW_5_BM  PIN1_bm   // PF1
#define ROW_5_VPORT VPORTF

#define ROW_6_BM  PIN2_bm   // PF2
#define ROW_6_VPORT VPORTF

#define ROW_7_BM  PIN3_bm   // PF3
#define ROW_7_VPORT VPORTF

#define ROW_8_BM  PIN4_bm   // PF4
#define ROW_8_VPORT VPORTF

#define ROW_9_BM  PIN5_bm   // PF5
#define ROW_9_VPORT VPORTF

#endif // MATRIX_H
```

---

## Example 3: Usage in matrix.c (Optimized Initialization)

### matrix.c

```c
#include "matrix.h"
#include "hal_gpio.h"

void matrix_init(void) {
    //=========================================================================
    // COLUMNS: Set as inputs with pull-ups (9 columns total)
    //=========================================================================
    
    // PORTA columns (PA0-PA3): 4 pins at once
    gpio_set_input_mask(&COL_PORTA_PORT, COL_PORTA_MASK);
    gpio_set_pullup_range(&COL_PORTA_PORT, 0, 3);  // Pins 0-3
    
    // PORTD columns (PD1-PD5): 5 pins at once
    gpio_set_input_mask(&COL_PORTD_PORT, COL_PORTD_MASK);
    gpio_set_pullup_range(&COL_PORTD_PORT, 1, 5);  // Pins 1-5
    
    //=========================================================================
    // ROWS: Set as outputs, initialize HIGH (10 rows total)
    //=========================================================================
    
    // PORTC rows (PC3): 1 pin
    gpio_set_output_mask(&ROW_PORTC_PORT, ROW_PORTC_MASK);
    gpio_write_high_mask(&ROW_PORTC_VPORT, ROW_PORTC_MASK);
    
    // PORTD rows (PD0, PD6, PD7): 3 pins at once
    gpio_set_output_mask(&ROW_PORTD_PORT, ROW_PORTD_MASK);
    gpio_write_high_mask(&ROW_PORTD_VPORT, ROW_PORTD_MASK);
    
    // PORTF rows (PF0-PF5): 6 pins at once ← BIGGEST WIN
    gpio_set_output_mask(&ROW_PORTF_PORT, ROW_PORTF_MASK);
    gpio_write_high_mask(&ROW_PORTF_VPORT, ROW_PORTF_MASK);
}
```

**Benefit**: Instead of 10 separate calls to set row directions and 10 calls to set them high, you have:
- 3 calls to set directions (one per port)
- 3 calls to set high (one per port)

---

## Example 4: Optimized Row Scanning (Two Approaches)

### Approach A: Traditional Sequential Scan (One Row at a Time)

```c
void matrix_scan(uint8_t *pressed_row, uint8_t *pressed_col) {
    *pressed_row = 0xFF;
    *pressed_col = 0xFF;
    
    for (uint8_t row = 0; row < 10; row++) {
        // Drive row LOW
        switch(row) {
            case 0: gpio_write_low(&ROW_0_VPORT, ROW_0_BM); break;
            case 1: gpio_write_low(&ROW_1_VPORT, ROW_1_BM); break;
            case 2: gpio_write_low(&ROW_2_VPORT, ROW_2_BM); break;
            case 3: gpio_write_low(&ROW_3_VPORT, ROW_3_BM); break;
            case 4: gpio_write_low(&ROW_4_VPORT, ROW_4_BM); break;
            case 5: gpio_write_low(&ROW_5_VPORT, ROW_5_BM); break;
            case 6: gpio_write_low(&ROW_6_VPORT, ROW_6_BM); break;
            case 7: gpio_write_low(&ROW_7_VPORT, ROW_7_BM); break;
            case 8: gpio_write_low(&ROW_8_VPORT, ROW_8_BM); break;
            case 9: gpio_write_low(&ROW_9_VPORT, ROW_9_BM); break;
        }
        
        // Small delay for signal to settle (optional, test if needed)
        // asm volatile("nop"); asm volatile("nop");
        
        // Read ALL columns at once using port masks
        uint8_t cols_porta = gpio_read_mask(&COL_PORTA_VPORT, COL_PORTA_MASK);
        uint8_t cols_portd = gpio_read_mask(&COL_PORTD_VPORT, COL_PORTD_MASK);
        
        // Check if any column is LOW (pressed, pull-ups mean LOW = pressed)
        if (~cols_porta & COL_PORTA_MASK) {
            // Key pressed in PORTA columns
            *pressed_row = row;
            // Determine which column
            for (uint8_t c = 0; c < 4; c++) {
                if (!(cols_porta & (1 << c))) {
                    *pressed_col = c;  // Columns 0-3
                    break;
                }
            }
        }
        
        if (~cols_portd & COL_PORTD_MASK) {
            // Key pressed in PORTD columns
            *pressed_row = row;
            // Determine which column (PD1-PD5 = columns 4-8)
            for (uint8_t c = 1; c <= 5; c++) {
                if (!(cols_portd & (1 << c))) {
                    *pressed_col = c + 3;  // Map PD1→col4, PD2→col5, etc.
                    break;
                }
            }
        }
        
        // Return row HIGH
        switch(row) {
            case 0: gpio_write_high(&ROW_0_VPORT, ROW_0_BM); break;
            case 1: gpio_write_high(&ROW_1_VPORT, ROW_1_BM); break;
            case 2: gpio_write_high(&ROW_2_VPORT, ROW_2_BM); break;
            case 3: gpio_write_high(&ROW_3_VPORT, ROW_3_BM); break;
            case 4: gpio_write_high(&ROW_4_VPORT, ROW_4_BM); break;
            case 5: gpio_write_high(&ROW_5_VPORT, ROW_5_BM); break;
            case 6: gpio_write_high(&ROW_6_VPORT, ROW_6_BM); break;
            case 7: gpio_write_high(&ROW_7_VPORT, ROW_7_BM); break;
            case 8: gpio_write_high(&ROW_8_VPORT, ROW_8_BM); break;
            case 9: gpio_write_high(&ROW_9_VPORT, ROW_9_BM); break;
        }
        
        // If key found, can return early (or continue to detect multiple)
        if (*pressed_row != 0xFF) {
            break;  // For 6KRO, you'd continue scanning and store multiple keys
        }
    }
}
```

**Optimization**: Reading columns happens with **2 register reads** (PORTA and PORTD), not 9 individual reads.

---

### Approach B: Ultra-Fast PORTF Scanning (6 Rows at Once)

For the 6 rows on PORTF (rows 4-9), you can read them **all simultaneously**:

```c
// Special fast scan for PORTF rows (rows 4-9)
void matrix_scan_portf_fast(uint8_t keys[6][9]) {
    // Set ALL PORTF rows LOW at once
    gpio_write_low_mask(&ROW_PORTF_VPORT, ROW_PORTF_MASK);
    
    // Read all columns
    uint8_t cols_porta = gpio_read_mask(&COL_PORTA_VPORT, COL_PORTA_MASK);
    uint8_t cols_portd = gpio_read_mask(&COL_PORTD_VPORT, COL_PORTD_MASK);
    
    // Now ALL keys in rows 4-9 that are pressed will show up as LOW
    // This is ONLY useful if you drive ALL rows low simultaneously
    // and use diodes to prevent ghosting
    
    // Return rows HIGH
    gpio_write_high_mask(&ROW_PORTF_VPORT, ROW_PORTF_MASK);
    
    // Note: This approach is tricky with standard matrix scanning
    // Better to stick with sequential scanning unless you implement
    // a different matrix reading strategy
}
```

**Reality Check**: This "all rows at once" approach doesn't work well with traditional matrix scanning because you can't tell which row a key is on. It's shown here for completeness, but **stick with sequential scanning** (Approach A).

---

## Example 5: Cleaner Column Reading Helper

### matrix.c (Helper Function)

```c
// Read all 9 columns, return as a 16-bit word with bits 0-8 set
// Bit LOW = key pressed (due to pull-ups)
static inline uint16_t matrix_read_all_columns(void) {
    uint16_t result = 0;
    
    // Read PORTA columns (PA0-PA3 = columns 0-3)
    uint8_t porta = gpio_read_mask(&COL_PORTA_VPORT, COL_PORTA_MASK);
    result |= (porta & 0x0F);  // Bits 0-3
    
    // Read PORTD columns (PD1-PD5 = columns 4-8)
    uint8_t portd = gpio_read_mask(&COL_PORTD_VPORT, COL_PORTD_MASK);
    // Shift PD1-PD5 to align with columns 4-8
    result |= ((portd >> 1) & 0x1F) << 4;  // Bits 4-8
    
    return ~result;  // Invert: 1 = pressed, 0 = not pressed
}

// Usage in scan loop:
void matrix_scan_optimized(uint8_t *pressed_row, uint8_t *pressed_col) {
    for (uint8_t row = 0; row < 10; row++) {
        // Drive row LOW (switch statement as before)
        // ...
        
        // Read all columns at once
        uint16_t cols = matrix_read_all_columns();
        
        if (cols != 0) {
            *pressed_row = row;
            // Find first set bit (first pressed column)
            for (uint8_t c = 0; c < 9; c++) {
                if (cols & (1 << c)) {
                    *pressed_col = c;
                    break;
                }
            }
        }
        
        // Return row HIGH (switch statement as before)
        // ...
        
        if (*pressed_row != 0xFF) break;
    }
}
```

---

## Summary of Optimizations

### 1. Port-Level Operations
```c
// Instead of:
gpio_set_output(&PORTF, PIN0_bm);
gpio_set_output(&PORTF, PIN1_bm);
gpio_set_output(&PORTF, PIN2_bm);
gpio_set_output(&PORTF, PIN3_bm);
gpio_set_output(&PORTF, PIN4_bm);
gpio_set_output(&PORTF, PIN5_bm);

// Do this:
gpio_set_output_mask(&PORTF, 0b00111111);  // One instruction
```

### 2. Column Reading
```c
// Instead of:
bool c0 = gpio_read(&VPORTA, PIN0_bm);
bool c1 = gpio_read(&VPORTA, PIN1_bm);
bool c2 = gpio_read(&VPORTA, PIN2_bm);
bool c3 = gpio_read(&VPORTA, PIN3_bm);

// Do this:
uint8_t cols = gpio_read_mask(&VPORTA, 0x0F);  // One read
// Then test individual bits
```

### 3. Key Functions to Add to HAL

```c
// Essential port-level operations:
gpio_set_output_mask()      // Set multiple pins as outputs
gpio_set_input_mask()       // Set multiple pins as inputs
gpio_write_high_mask()      // Set multiple pins HIGH
gpio_write_low_mask()       // Set multiple pins LOW
gpio_read_mask()            // Read multiple pins
gpio_set_pullup_range()     // Configure pull-ups for range
```

---

## Performance Impact

**Before** (individual pin operations):
- 10 writes to set row directions (initialization)
- 10 writes to set rows HIGH (initialization)
- 10 reads per scan cycle (one per column, worst case)

**After** (port-level operations):
- 3 writes to set row directions (one per port)
- 3 writes to set rows HIGH (one per port)
- 2 reads per scan cycle (PORTA + PORTD columns)

**Result**: 
- ~3× faster initialization
- ~5× faster column reading per row
- Cleaner, more maintainable code

This is the sweet spot for "light HAL" - just enough abstraction to express port-level operations clearly without hiding what's happening.



## Prompt:
12/30/2025, 11:27:03 PM

Is it common practice to ‘calculate’ a bit mask such as 0b11000001 (port D rows) in a define (macro) by OR ing different single pin ‘constants’?



## Response:
12/30/2025, 11:28:02 PM

# Bit Mask Construction in Macros

## Yes, This Is Very Common Practice!

Using `|` (OR) to build bitmasks from individual pin constants is **standard embedded practice** and has several advantages.

---

## Example: Your PORTD Rows

### Current Way (Manual Calculation)
```c
// PORTD rows: PD0, PD6, PD7 (3 pins)
#define ROW_PORTD_MASK  0b11000001  // Pins 0, 6, 7
```

**Problem**: Have to manually calculate and it's error-prone. What if you add/remove a pin later?

### Better Way (Composed from Pin Constants)
```c
// PORTD rows: PD0, PD6, PD7 (3 pins)
#define ROW_PORTD_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)
```

**Advantages**:
- ✅ Self-documenting (shows which pins)
- ✅ Less error-prone (no manual binary calculation)
- ✅ Easy to modify (add/remove pins)
- ✅ Compiler optimizes to same constant
- ✅ Matches datasheet naming (`PIN0_bm`, `PIN6_bm`)

---

## Complete Example for Your Design

### matrix.h (Recommended Style)

```c
#ifndef MATRIX_H
#define MATRIX_H

#include <avr/io.h>

//=============================================================================
// COLUMN PIN DEFINITIONS (Inputs with pull-ups)
//=============================================================================

// PORTA columns: PA0, PA1, PA2, PA3 (4 pins)
#define COL_PORTA_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm)
#define COL_PORTA_PORT  PORTA
#define COL_PORTA_VPORT VPORTA

// PORTD columns: PD1, PD2, PD3, PD4, PD5 (5 pins)
#define COL_PORTD_MASK  (PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)
#define COL_PORTD_PORT  PORTD
#define COL_PORTD_VPORT VPORTD

//=============================================================================
// ROW PIN DEFINITIONS (Outputs, normally high)
//=============================================================================

// PORTC rows: PC3 (1 pin)
#define ROW_PORTC_MASK  (PIN3_bm)
#define ROW_PORTC_PORT  PORTC
#define ROW_PORTC_VPORT VPORTC

// PORTD rows: PD0, PD6, PD7 (3 pins)
#define ROW_PORTD_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)
#define ROW_PORTD_PORT  PORTD
#define ROW_PORTD_VPORT VPORTD

// PORTF rows: PF0, PF1, PF2, PF3, PF4, PF5 (6 pins)
#define ROW_PORTF_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)
#define ROW_PORTF_PORT  PORTF
#define ROW_PORTF_VPORT VPORTF

//=============================================================================
// INDIVIDUAL ROW PINS (for sequential scanning)
//=============================================================================

#define ROW_0_BM    PIN3_bm      // PC3
#define ROW_0_VPORT VPORTC

#define ROW_1_BM    PIN0_bm      // PD0
#define ROW_1_VPORT VPORTD

#define ROW_2_BM    PIN6_bm      // PD6
#define ROW_2_VPORT VPORTD

#define ROW_3_BM    PIN7_bm      // PD7
#define ROW_3_VPORT VPORTD

#define ROW_4_BM    PIN0_bm      // PF0
#define ROW_4_VPORT VPORTF

#define ROW_5_BM    PIN1_bm      // PF1
#define ROW_5_VPORT VPORTF

#define ROW_6_BM    PIN2_bm      // PF2
#define ROW_6_VPORT VPORTF

#define ROW_7_BM    PIN3_bm      // PF3
#define ROW_7_VPORT VPORTF

#define ROW_8_BM    PIN4_bm      // PF4
#define ROW_8_VPORT VPORTF

#define ROW_9_BM    PIN5_bm      // PF5
#define ROW_9_VPORT VPORTF

#endif // MATRIX_H
```

---

## Why This Is Standard Practice

### 1. **AVR/Atmel Does This Everywhere**

From `<avr/io.h>` and device-specific headers:

```c
// Actual definitions from AVR headers:
#define PIN0_bm  0x01  /* Pin 0 bit mask */
#define PIN1_bm  0x02  /* Pin 1 bit mask */
#define PIN2_bm  0x04  /* Pin 2 bit mask */
#define PIN3_bm  0x08  /* Pin 3 bit mask */
// ... etc

// Common pattern in AVR code:
#define MOTOR_PINS  (PIN2_bm | PIN3_bm | PIN4_bm)
```

### 2. **ARM CMSIS Does This**

```c
// From ARM CMSIS headers:
#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)

// Usage:
#define LED_PINS  (GPIO_PIN_5 | GPIO_PIN_6)
```

### 3. **QMK Keyboard Firmware Does This**

```c
// From QMK:
#define MATRIX_ROW_PINS { B0, B1, B2, B3 }
#define MATRIX_COL_PINS { C0, C1, C2, C3 }

// And in port-based implementations:
#define ROW_MASK  (1<<0 | 1<<1 | 1<<2 | 1<<3)
```

---

## Compiler Optimization

The compiler **completely optimizes** composed bitmasks to constants:

### Your Code:
```c
#define ROW_PORTF_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)

void matrix_init(void) {
    gpio_write_high_mask(&ROW_PORTF_VPORT, ROW_PORTF_MASK);
}
```

### What Compiler Generates (Assembly):
```asm
; Compiler knows ROW_PORTF_MASK = 0x3F at compile time
ldi  r24, 0x3F       ; Load immediate 0b00111111
out  VPORTF_OUT, r24 ; Write to port
```

**Zero runtime cost** - it's as if you wrote `0x3F` directly.

---

## Additional Benefits

### Easy to Modify

```c
// Original:
#define ROW_PORTF_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)

// Later, add PIN7:
#define ROW_PORTF_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm | PIN7_bm)

// vs manually recalculating:
#define ROW_PORTF_MASK  0b10111111  // Easy to make a mistake!
```

### Self-Documenting Code

```c
// Clear which pins are used:
#define ENCODER_PINS  (PIN6_bm | PIN7_bm)  // PA6, PA7

// vs unclear:
#define ENCODER_PINS  0xC0  // What pins are these? Need to calculate.
```

### Consistent with AVR Style

```c
// This is how AVR peripheral initialization is done:
USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;  // Enable RX and TX

// Not:
USART0.CTRLB = 0xC0;  // What does this enable?
```

---

## Variations You Might See

### 1. Using Shift Operators (Older Style)
```c
// Older style, still valid:
#define ROW_PORTD_MASK  ((1<<0) | (1<<6) | (1<<7))

// Modern AVR style (preferred):
#define ROW_PORTD_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)
```

Both work, but `PIN0_bm` style is more modern and matches AVR naming.

### 2. Parentheses Are Important
```c
// ALWAYS use parentheses around the entire expression:
#define ROW_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)  // ✅ Good

// Without parens can cause issues:
#define ROW_MASK  PIN0_bm | PIN6_bm | PIN7_bm    // ⚠️ Risky

// Example of problem:
uint8_t test = ROW_MASK & 0xFF;  
// Expands to: PIN0_bm | PIN6_bm | PIN7_bm & 0xFF
// Due to precedence, this is: PIN0_bm | PIN6_bm | (PIN7_bm & 0xFF)
// NOT what you wanted!

// With parens:
uint8_t test = ROW_MASK & 0xFF;
// Expands to: (PIN0_bm | PIN6_bm | PIN7_bm) & 0xFF
// Correct!
```

### 3. Contiguous vs Non-Contiguous Pins
```c
// Contiguous (PF0-PF5):
#define ROW_PORTF_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)
// Could also write as: 0x3F or ((1<<6)-1) but pin names are clearer

// Non-contiguous (PD0, PD6, PD7):
#define ROW_PORTD_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)
// Can ONLY be written this way (or 0xC1), pin names much clearer
```

---

## Real-World Examples

### From Linux Kernel
```c
// drivers/gpio/gpio-pca953x.c
#define PCA_INT		0x0100
#define PCA_PCAL	0x0200
#define PCA_LATCH_INT	(PCA_PCAL | PCA_INT)  // Composed!
```

### From FreeRTOS
```c
// Interrupt priority grouping
#define prioritygroup4  (7UL)
#define prioritygroup3  (6UL)
#define NVIC_PRIORITY   (prioritygroup4 | prioritygroup3)
```

### From AVR-libc Examples
```c
// Setting up timer:
TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS11);  // CTC mode, prescaler 64

// Modern equivalent:
TCCR1B = (TIMER_WGM12_bm | TIMER_CS10_bm | TIMER_CS11_bm);
```

---

## Best Practices Summary

### ✅ DO:
```c
#define ROW_PORTD_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)
#define ALL_ROWS        (ROW_PORTC_MASK | ROW_PORTD_MASK | ROW_PORTF_MASK)
#define TIMER_CONFIG    (TCA_ENABLE_bm | TCA_RUNSTBY_bm)
```

### ❌ DON'T:
```c
#define ROW_PORTD_MASK  0xC1              // Hard to verify
#define ROW_PORTD_MASK  PIN0_bm|PIN6_bm   // Missing outer parentheses
#define ALL_ROWS        0xFF              // Doesn't show which ports
```

### 🤔 OPTIONAL (But Less Clear):
```c
#define ROW_PORTF_MASK  0b00111111        // Binary literal OK but less flexible
#define ROW_PORTD_MASK  ((1<<0)|(1<<6)|(1<<7))  // Old style, still valid
```

---

## Conclusion

**Yes, absolutely use composed bitmasks!**

```c
// Your final matrix.h should use:
#define COL_PORTA_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm)
#define COL_PORTD_MASK  (PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)
#define ROW_PORTC_MASK  (PIN3_bm)
#define ROW_PORTD_MASK  (PIN0_bm | PIN6_bm | PIN7_bm)
#define ROW_PORTF_MASK  (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm)
```

This is:
- **Standard practice** across embedded development
- **Self-documenting** (shows which pins)
- **Compiler-optimized** (zero runtime cost)
- **Easy to maintain** (add/remove pins easily)
- **Less error-prone** (no manual binary calculations)

It's exactly what professional embedded developers do, and it's the right choice for AmberClick90.





---
Powered by [Claude Exporter](https://www.ai-chat-exporter.net)