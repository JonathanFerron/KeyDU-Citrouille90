# Citrouille90 — Firmware Reference

**Project:** Citrouille90 (AVRDU.Enc.Choc)  
**MCU:** AVR64DU32 (TQFP-32), 64 KB flash, 8 KB SRAM, 24 MHz  
**Document purpose:** Personal working reference. Consolidates all firmware decisions and provides a sequenced build plan.

---

## 1. Core Philosophy

Bare-metal, intentional, minimal. Every line of code is understood before it is committed. Borrow ideas from QMK, LUFA, DxCore, MCC, TinyUSB — but only what applies cleanly to the AVR64DU32 and the specific needs of this keyboard. No framework overhead, no dead code, no preprocessor complexity.

---

## 2. Coding Style

### 2.1 Naming

- Functions and variables: `snake_case`
- `#define` constants: `UPPER_CASE`
- Types: `snake_case_t` (e.g. `keycode_t`, `matrix_state_t`)
- Enums: `typedef enum { UPPER_CASE_MEMBERS } name_t;`
- No PascalCase anywhere

### 2.2 Header / Source Split

- `.h` is the contract: type definitions, public prototypes, `#define` constants that callers need, minimized `extern` declarations.
- `.c` is the implementation: `static` state, internal helpers, ISR bodies, hardware pin definitions, port bitmasks.
- All module-level state variables and internal functions are `static`.
- `static inline` for small hot-path helpers with no external callers (e.g. `scan_col()` in `matrix.c`).
- `gpio.h` is the one exception: header-only, all macros, no `.c` counterpart.

### 2.3 Comments

- Register operation lines: brief inline comment on the *why*, not the *what*.
- Public functions in `.h`: short plain-English description. No Doxygen.
- Section separators: `/* --- */` block style.
- Step-numbered comments encouraged in non-trivial state machines.
- Hardware rationale (RC filter, pull-up decisions, timing) belongs in the relevant `init()` function or at the top of the `.c` file.

### 2.4 Error Handling

No return codes from hardware drivers. Init functions configure registers and return. `_Static_assert` for compile-time size and range checks where useful. If hardware init is wrong, the firmware does not work — debug at the register level.

---

## 3. Keycode System

Defined in `keycode.h` (header-only, no `.c` counterpart).

### 3.1 Base Type

`uint16_t` throughout — keymaps, lookup functions, report logic. Upper byte encodes category; lower byte encodes value within range.

### 3.2 Ranges and Prefixes

| Prefix | Range            | Category                     |
| ------ | ---------------- | ---------------------------- |
| `KC_`  | `0x0000–0x00FF`  | HID keyboard (Page 0x07)     |
| `CC_`  | `0x0100–0x01FF`  | Consumer control (Page 0x0C) |
| `LY_`  | `0x0200–0x02FF`  | Layer switching              |
| `MC_`  | `0x0300–0x03FF`  | Macros                       |
| `LD_`  | `0x0400–0x04FF`  | LED control                  |
| `SYS_` | `0x0500–0x05FF`  | System / firmware            |
| CP     | 0x0600 to 0x06FF | Compose.                     |

- `KC_NO = 0x0000` — no event (HID Usage 0x00 is reserved/safe)

- `KC_TRNS = 0x0001` — transparent, falls through to lower layer. **Not** `0xFF` — that is a valid HID keycode (Keyboard Application).

- `______` and `XXXXXXX` aliases defined in `keycode.h`:
  
  ```c
  #define ______   KC_TRNS   /* transparent — falls through */
  #define XXXXXXX  KC_NO     /* explicitly disabled         */
  ```

- Each `_BASE` sentinel equals the first value of its range (e.g. `CC_BASE = 0x0100`). Individual codes start at `_BASE | 0x01`, so no code ever equals its range sentinel.

### 3.3 Dispatch Predicates

Dispatch in the key processing loop uses `IS_*` predicate macros and the `0xFF00` mask — no magic inline comparisons:

```c
#define IS_CONSUMER_KEY(kc)  (((kc) & 0xFF00) == 0x0100)
#define IS_LAYER_KEY(kc)     (((kc) & 0xFF00) == 0x0200)
#define IS_MACRO_KEY(kc)     (((kc) & 0xFF00) == 0x0300)
#define IS_LED_KEY(kc)       (((kc) & 0xFF00) == 0x0400)
#define IS_SYS_KEY(kc)       (((kc) & 0xFF00) == 0x0500)
#define IS_COMPOSE_KEY(kc)   (((kc) & 0xFF00) == 0x0600)
#define GET_LAYER(kc)        ((kc) & 0x00FF)
#define MOD_BIT(kc)          /* converts KC_Lxxx modifier to HID bitmask */
```

---

## 4. Module Architecture

### 4.1 Compiled Source Files (`firmware/KeyDU.App/` on KeyDU-Citrouille90 git repo)

| File            | Owns                                                                       |
| --------------- | -------------------------------------------------------------------------- |
| `gpio.h`        | GPIO HAL — header-only, all macros                                         |
| `matrix.c/.h`   | Matrix init and scan; `register_key()` callback to keyboard layer          |
| `encoder.c/.h`  | Encoder init and hardware scan only; `encoder_step()` stub                 |
| `keycode.h`     | All keycode `#define`s, `IS_*` predicates, aliases                         |
| `led.c/.h`      | TCA0 PWM init and brightness control                                       |
| `keymap.c/.h`   | PROGMEM keymap array, layer lookup, `encoder_step()` impl, `keymap_tick()` |
| `macro.c/.h`    | Macro dispatch and PROGMEM sequences                                       |
| `keyboard.c/.h` | Main keyboard logic, `keyboard_task()`, key processing loop                |
| `main.c`        | Entry point, system init, TCB0 gate loop                                   |
| `usb_hid.c/.h`  | Non-blocking report buffering, HID report structs, EP management           |
| `usb_desc.c/.h` | All USB descriptor tables                                                  |

USB stack shared library (`firmware/usbcore/` — compiled into both App and BL):

| File                  | Owns                                                      |
| --------------------- | --------------------------------------------------------- |
| `usb_ctrl.c/.h`       | USB device state machine, EP0 control transfers           |
| `usb_ep.c/.h`         | Hardware endpoint table, FIFO pairs, primitive read/write |
| `usb_ep_stream.c/.h`  | Stream helpers (ep_read_*, ep_write_*)                    |

### 4.2 Bootloader (`firmware/KeyDU.BL/`)

| File              | Owns                                             |
| ----------------- | ------------------------------------------------ |
| `main.c`          | Early EEPROM check, jump logic, BL state machine |
| `usb_vendor.c/.h` | Vendor class USB, flash write protocol           |
| `flash.c/.h`      | SPM flash write routines                         |
| `linker.ld`       | Bootloader-specific linker script                |

### 4.3 Not Compiled (`KeyDU-Examples` separate git repo — planned, not yet created)

Reference implementations for macros and encoder behaviors. Copy desired functions into `KeyDU.App/macro.c` or `KeyDU.App/encoder.c` and compile from there. No preprocessor flags, no dead code.

```
/
  macros/
    basic/       copy_paste.c, text_editing.c, navigation.c
    media/       playback.c, volume.c
    window/      alt_tab.c, virtual_desktops.c
    development/ ide_shortcuts.c, terminal.c
  encoders/
    basic_volume.c, smart_alt_tab.c, scroll_wheel.c, brightness.c
```

---

## 5. GPIO HAL

Defined entirely in `gpio.h` (header-only). No runtime overhead — all macros expand directly to register operations.

### 5.1 Pin Descriptor

```c
typedef struct { PORT_t *port; uint8_t pin_n; } gpio_pin_t;
#define GPIO_PIN(port, pin_n)  { &(port), (pin_n) }
```

### 5.2 Output Operations

```c
#define GPIO_HIGH(p)    ((p).port->OUTSET = (1u << (p).pin_n))
#define GPIO_LOW(p)     ((p).port->OUTCLR = (1u << (p).pin_n))
#define GPIO_TOGGLE(p)  ((p).port->OUTTGL = (1u << (p).pin_n))
```

`OUTSET/OUTCLR/OUTTGL` are single-operation and atomic — preferred over read-modify-write for outputs.

### 5.3 Input Read

```c
#define GPIO_READ(p)  (((p).port->IN >> (p).pin_n) & 1u)
```

For the hot scan path, use VPORT instead (single-cycle `IN` instruction):

```c
/* vport: VPORTA / VPORTC / VPORTD / VPORTF   pin_n: 0–7 */
#define GPIO_VPORT_READ(vport, pin_n)  (((vport).IN >> (pin_n)) & 1u)
```

### 5.4 Multi-Pin Configuration

`PINCONFIG` is mirrored across all ports — write once, apply to any port's `PINCTRLSET`:

```c
#define GPIO_MULTIPIN_PULLUP(port, mask) \
    do { (port).PINCONFIG = PORT_PULLUPEN_bm; \
         (port).PINCTRLSET = (mask); } while (0)

#define GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(port, mask) \
    do { (port).PINCONFIG = PORT_ISC_INPUT_DISABLE_gc; \
         (port).PINCTRLSET = (mask); } while (0)
```

Use `PINCTRLUPD` (not `PINCTRLSET`) when overwriting the full `PINnCTRL`, since `PINCTRLSET` ORs into existing ISC bits.

### 5.5 VPORT Availability on AVR64DU32 (32-pin)

| Port  | VPORT  | Notes                                |
| ----- | ------ | ------------------------------------ |
| PORTA | VPORTA | PA0–PA3, PA6 rows; PA7 encoder A     |
| PORTC | VPORTC | PC3 encoder B                        |
| PORTD | VPORTD | PD0–PD4 rows; PD5–PD7 column outputs |
| PORTF | VPORTF | PF0–PF5 column outputs               |

No PORTB on this device.

### 5.6 VPORT Collision Warning

Do not access a VPORT register immediately after accessing the corresponding regular PORT register. Ensure at least one unrelated instruction separates a PORT write (in init) from the first VPORT read at runtime.

### 5.7 Single Source of Truth for Pin Assignments

When a pin is accessed via both a `gpio_pin_t` (for init) and VPORT (for scan hot path), define port and pin number once:

```c
#define ENC_A_PORT   PORTA
#define ENC_A_VPORT  VPORTA
#define ENC_A_PIN    7
#define ENC_B_PORT   PORTC
#define ENC_B_VPORT  VPORTC
#define ENC_B_PIN    3

static const gpio_pin_t ENC_A = GPIO_PIN(ENC_A_PORT, ENC_A_PIN);
static const gpio_pin_t ENC_B = GPIO_PIN(ENC_B_PORT, ENC_B_PIN);

/* Hot path */
uint8_t curr_ab = (GPIO_VPORT_READ(ENC_A_VPORT, ENC_A_PIN) << 1)
                |  GPIO_VPORT_READ(ENC_B_VPORT, ENC_B_PIN);
```

---

## 6. Matrix Scanning

### 6.1 Parameters

- **Layout:** 10 rows × 9 columns = 90 keys
- **Electrical scan direction:** Drive column low, read rows (pull-up on rows)
- **Scan rate:** 1000 Hz (1 ms interval), gated by TCB0 in main loop
- **Diode orientation:** one diode per switch, cathode toward column

### 6.2 Rationale for 1 kHz

USB FS HID polls every 1 ms — scanning at 1 kHz aligns cleanly with the report deadline. A full 90-key scan completes well under 100 µs at 24 MHz, leaving >90% of the 1 ms window for USB and other tasks. 1 kHz is a deliberate architectural choice for determinism.

### 6.3 Port Allocation

Rows are pulled up via `GPIO_MULTIPIN_PULLUP`. Column output pins have input buffers disabled via `GPIO_MULTIPIN_DISABLE_INPUT_BUFFER` to reduce power and noise.

```c
GPIO_MULTIPIN_PULLUP(PORTA, ROW_A_MASK);
GPIO_MULTIPIN_PULLUP(PORTD, ROW_D_MASK);
GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTF, COL_F_MASK);
GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTD, COL_D_MASK);
```

### 6.4 VPORT Scan Idiom

```c
/* Single IN per port — reads full byte atomically */
uint8_t rows_d = ~VPORTD.IN & ROW_D_MASK;
uint8_t rows_a = ~VPORTA.IN & ROW_A_MASK;
```

### 6.5 Physical → Electrical Mapping

The `KEYMAP()` macro remaps the 20×5 physical layout to the 9×10 electrical matrix. Implement with `uint16_t` types based on final wiring from PCB Kicad file.

---

## 7. Debounce

**Decision:** 4 successive identical readings (including the first) for both press and release → **3 ms debounce window** at 1 kHz scan rate.

**Rationale:** Kailh Choc V1 datasheet specifies bounce time under 5 ms. 3 ms provides margin while being simpler than eager-press/deferred-release. Revisit empirically during firmware testing if needed.

Implementation lives in `matrix.c` — each key slot has a `uint8_t` debounce counter. State only changes when the counter reaches threshold.

---

## 8. Encoder

### 8.1 Hardware Signal Path

- PES12 encoder, no integrated push switch
- Encoder pins: plain input, **no internal pull-up** (external RC filter sets the operating point)
- RC filter per pin (Bourns-recommended): 10 kΩ pull-up to VDD, 10 kΩ series, 10 nF to GND, terminal C to GND
- RC time constant: 10 kΩ × 10 nF = **100 µs** — suppresses contact bounce electrically
- 100 µs is well below the 1 ms scan interval; legitimate quadrature transitions are unaffected

### 8.2 PES12 Relevant Specs

| Parameter                         | Value    |
| --------------------------------- | -------- |
| Max contact bounce                | 5 ms     |
| Max rotation speed                | 60 RPM   |
| Detents per revolution            | 24       |
| Detent-to-detent time at 60 RPM   | ~41.7 ms |
| Quadrature transitions per detent | 4        |
| Min time between transitions      | ~10.4 ms |

### 8.3 Scanning

Encoder pins scanned as part of the regular 1 kHz loop in `encoder.c` — not via interrupts. With hardware RC filtering in place the signal is clean and stable well within 1 ms. Integrating into the existing loop keeps the architecture unified and deterministic.

### 8.4 Quadrature Decode Algorithm (`encoder.c`)

16-entry LUT state machine. Software layers retained after hardware RC filter:

- 16-entry LUT — non-optional, core decode
- ±4 accumulator threshold — requires full valid quadrature cycle before emitting a step
- Accumulator reset on illegal transition — belt-and-suspenders against residual noise
- Post-step lockout timer (`LOCKOUT_MS = 2`) — guards against ringing after a valid step

```c
/* index = (prev_ab << 2) | curr_ab
   +1 = CW,  -1 = CCW,  0 = no change or illegal */
static const int8_t enc_lut[16] = {
     0,  /* 00→00 no change  */
    -1,  /* 00→01 valid CCW  */
    +1,  /* 00→10 valid CW   */
     0,  /* 00→11 illegal    */
    +1,  /* 01→00 valid CW   */
     0,  /* 01→01 no change  */
     0,  /* 01→10 illegal    */
    -1,  /* 01→11 valid CCW  */
    -1,  /* 10→00 valid CCW  */
     0,  /* 10→01 illegal    */
     0,  /* 10→10 no change  */
    +1,  /* 10→11 valid CW   */
     0,  /* 11→00 illegal    */
    +1,  /* 11→01 valid CW   */  
    -1,  /* 11→10 valid CCW  */ 
     0,  /* 11→11 no change  */
};
```

**Key invariants:**

- `prev_ab` is NOT updated on illegal transition — anchors to last known good state
- Lockout timer is decremented before the lockout check — if it reaches zero, decode runs in the same cycle with no extra delay
- Accumulator is reset before `emit_step()` — new cycle always starts clean
- `emit_step()` dispatches to `encoder_step()` in `keymap.c`, which is layer-aware

### 8.5 Module Boundary

`encoder.c` owns: hardware pin read, LUT decode, accumulator, lockout, calling `encoder_step()`.  
`keymap.c` owns: `encoder_step(int8_t dir)` implementation — maps CW/CCW to keycode actions per active layer.

This boundary is deliberate. `encoder_step()` needs layer context which lives in `keymap.c`. Moving behavior to `encoder.c` would create a circular dependency or force an ugly callback.

---

## 9. Keymap and Layer Logic

### 9.1 Keymap Array

```c
const uint16_t PROGMEM keymaps[NUM_LAYERS][MATRIX_ROWS][MATRIX_COLS];
```

- Indexed `[layer][row][col]` — row-major, matching scan loop order
- `NUM_LAYERS = 3` (layers 0, 1, 2)
- Transparent fallthrough in `keymap_key_to_keycode()` walks down layers; returns `KC_NO` when layer 0 is also transparent
- Bounds check on entry: return `KC_NO` for out-of-range indices
- Physical→electrical remap via `KEYMAP()` macro — implemented with `uint16_t`

### 9.2 Layer Summary

| Layer | Name     | Access                     | LED state                         |
| ----- | -------- | -------------------------- | --------------------------------- |
| 0     | Base     | Default                    | Both at base brightness           |
| 1     | Function | Hold Fn (LY_01) on Layer 0 | LED1 brighter (+64), LED2 at base |
| 2     | System   | Hold LY_02 key on Layer 1  | Both brighter (+64)               |

Layer 2 contains: `SYS_BOOT` (enter bootloader), `SYS_RESET` (plain restart), `LD_BRIU`, `LD_BRID` (brightness adjust). Only one layer active at a time; highest active index wins at key press time.

Keymap invariant — layer 1 and 2:
All modifier key positions on layers 1 and 2 must be KC_TRNS. This enables both LY_01 → mod → key and mod → LY_01 → key orderings for modifier+function combos (e.g. Alt+F8). Leaving a modifier as KC_NO on an upper layer silently breaks that mod for all layer-1/2 combos. Enforce this as a design rule, not just convention.

**Keymap rule to document in `keymap.c`**

All modifier positions on layers 1 and 2 must be `KC_TRNS`. Add a comment block above the layer 1 definition stating this explicitly as a design invariant, not just a convention.

---

## 10. Macro System

### 10.1 Structure

- Macro sequences stored in PROGMEM in `macro.c`
- Dispatch via `execute_macro(uint16_t mc_keycode)` called from `keyboard.c` key processing loop
- Simple modifier+key macros use `send_mod_key()` helper
- Sequence macros use `run_macro_sequence()` stepping through a PROGMEM `macro_action_t` array

### 10.2 Adding a Simple Macro

```c
/* keycode.h */
#define MC_XL_VBA   (MC_BASE | 0x08)   /* Alt+F8 */

/* macro.c */
case MC_XL_VBA:
    send_mod_key(MOD_LALT, KC_F8);
    break;
```

### 10.3 Adding a Sequence Macro

```c
/* keycode.h */
#define MC_XL_PSTSP  (MC_BASE | 0x09)  /* Alt, E, S */

/* macro.c */
static const macro_action_t PROGMEM macro_excel_paste_special[] = {
    MT(KC_LALT), MT(KC_E), MT(KC_S), MACRO_END
};

case MC_XL_PSTSP:
    run_macro_sequence(macro_excel_paste_special);
    break;
```

### 10.4 Private Aliases

If `macro.c` needs temporary short aliases for `KC_` names that would collide with `keycode.h`, define them locally and `#undef` at the end of the file.

---

## 11. USB Architecture

### 11.1 Composite HID Device (Application Mode)

Two interfaces for v1 production firmware. Boot mouse interface deferred.

| Endpoint | Direction | Interface   | Purpose                              |
| -------- | --------- | ----------- | ------------------------------------ |
| EP0      | Bidir     | —           | Control: enumeration, SET_REPORT     |
| EP1 IN   | IN        | Interface 0 | Keyboard HID reports                 |
| EP1 OUT  | OUT       | Interface 0 | LED output reports (Num/Caps/Scroll) |
| EP2 IN   | IN        | Interface 1 | Consumer/extended control reports    |

EP1 IN and EP1 OUT share the endpoint number — IN and OUT are separate pipes on the same number, saves an endpoint slot.

### 11.2 Interface 0 — Boot Keyboard

- `bInterfaceSubClass: 0x01` (Boot Interface), `bInterfaceProtocol: 0x01` (Keyboard)
- UEFI/BIOS compatible
- 8-byte boot keyboard report: 1 byte modifiers + 1 byte reserved + 6 bytes keycodes (6KRO)

```c
typedef struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} hid_keyboard_report_t;
```

### 11.3 Interface 0 — LED Output Report

Delivered by host via EP1 OUT interrupt endpoint OR `SET_REPORT` control transfer on EP0. Support both. Most OSes prefer `SET_REPORT`. No Report ID (Interface 0 has no report IDs).

```
Bit 0: Num Lock
Bit 1: Caps Lock
Bit 2: Scroll Lock
Bit 3: Compose
Bit 4: Kana
Bits 5–7: Padding
```

### 11.4 Interface 1 — Extended Controls

- `bInterfaceSubClass: 0x00`, `bInterfaceProtocol: 0x00`
- OS only (not UEFI-visible — expected and acceptable)
- 4-byte report:

```
Byte 0:     Report ID (0x01 — Interface 1 needs a Report ID since it has no boot protocol)
Bytes 1–2:  Consumer code 16-bit (0x0000 = none) — HID Page 0x0C
Byte 3 b0:  System Sleep (HID Page 0x01, usage 0x82)
Byte 3 b1–7: Padding
```

Consumer codes currently implemented: Volume Up/Down/Mute, Play/Pause, Stop, Next/Prev Track.
Planned (see To Dos): Media Select, Launch Mail/Calculator/My Computer, WWW Search/Home/Back/Forward/Stop/Refresh/Favorites (0x0C page), and System Sleep (GD 0x01:0x82 via the existing system byte in the report).

### 11.5 SOF Interrupt and Scan Loop Relationship

`USB_SOF_vect` fires every 1 ms when host is connected and not suspended.

- **Primary role:** signals the host that a new report is ready (hid_flush() is called from the main loop before usb_ctrl_poll(); the SOF interrupt itself is kept enabled as a future phase-lock hook)
- **Secondary role (future):** snapshot `TCB0.CNT` for phase measurement
- **Not** the scan trigger — scan is triggered by TCB0

TCB0 runs unconditionally. During USB suspend or before enumeration, SOF stops but `keyboard_task()` keeps running from TCB0 regardless. No special handling needed in scan code.

**Correct HID report sequence:** Scan → queue report → send on next host SOF poll. Report reflects the world just before the host asked, not up to 1 ms ago.

### 11.6 Future Optimization — SOF Phase Lock

After USB stack is stable and scan duration is measured on real hardware:

```
TCB0 fires at (SOF_phase - scan_budget):
    → matrix_scan() → encoder_scan() → keymap_tick()
    → kbd_stage() puts report in queue

Main loop (continuous):
    → hid_flush() drains queue to EP1 IN when ep_in_ready()

SOF ISR fires ~scan_budget ms later:
    → snapshot TCB0.CNT for phase measurement (empty for now)
```

Phase measurement: snapshot `TCB0.CNT` in `usb_event_sof()`, average 8–16 samples. Adjust TCB0 compare register. Measure scan budget with GPIO toggle + logic analyzer (target < 0.2 ms / 4800 counts at 24 MHz).

**Prerequisites:** USB enumeration and basic keystroke reporting are confirmed working. Assume a conservative scan budget of 200 µs (< 0.2 ms) without measuring — tighten later with a GPIO toggle + logic analyzer if needed.

As a first guess, just assume a conservative scan budget of say 200 microseconds.

### 11.7 Bootloader Mode — Vendor Class

Vendor-defined class, single interface, EP0-only (no bulk endpoints). Five control-transfer commands (`VCMD_ERASE`, `VCMD_WRITE`, `VCMD_READ`, `VCMD_RESET`, `VCMD_STATUS`). Host tool: `keydu_flasher.c` (LibUSB 1.0). See §12 for full bootloader architecture.

### 11.8 Endpoint Table — FIFOEN=1 Layout

`USB0.CTRLA` is initialised with `FIFOEN` set. This prepends a 32-byte hardware FIFO ring buffer **before** the endpoint pairs in the table struct. `EP[0]` is at `EPPTR+32`, not `EPPTR+0`. The `ep_hw_table_t` struct in `usb_ep.h` models this with a `uint8_t fifo[32]` member at offset 0:

```c
typedef struct USB_PACKED {
    uint8_t       fifo[32];              /* transaction-complete FIFO — must be first */
    USB_EP_PAIR_t endpoints[EP_TABLE_COUNT];
    uint16_t      frame_num;
} ep_hw_table_t;
```

**Do not remove or reorder the `fifo` field.** The hardware FIFO layout is fixed by silicon; the struct must match it exactly.

`EP_TABLE_COUNT` is set per-target via `-DEP_TABLE_COUNT=N` in the makefile (App = 3, BL = 1). This sizes both the hardware table struct and the software FIFO array `ep_fifo_pair_t[EP_TABLE_COUNT]`.

### 11.9 EP0 Control Completion Signaling (FIFOEN=1)

With FIFOEN enabled, transaction-complete signalling for `TYPE=CONTROL` endpoints comes through `STATUS.TRNCOMPL` in the endpoint table entry, not exclusively via `INTFLAGSB`. The `USB0_TRNCOMPL_vect` ISR reads `USB0.FIFORP`, processes the FIFO entry, and latches completion into `usb_ep_trncompl_in` / `usb_ep_trncompl_out` bitmasks. Polling code in `ep_in_ready()` / `ep_out_received()` checks both paths.

Do not write back to `USB0.FIFORP` — hardware auto-manages FIFORP advancement. Process one entry per ISR invocation; hardware re-asserts TRNCOMPL if `FIFOWP != FIFORP` after advancement.

### 11.10 Bus-Reset ISR Requirements

On a USB bus reset, `USB0.INTCTRLB` (which enables TRNCOMPL interrupts) is cleared by hardware. The bus-reset ISR must:

1. Sync `USB0.FIFORP` to `USB0.FIFOWP` — discard stale pre-reset FIFO entries.
2. Call `ep_configure()` — reinitialise the endpoint table.
3. Re-enable `USB0.INTCTRLB = USB_TRNCOMPL_bm` after `ep_configure()`.

Omitting step 3 silences all future TRNCOMPL interrupts; EP0 NAKs every subsequent control transfer.

### 11.11 USB Stack Module Responsibilities

| Module            | Owns                                                                 |
| ----------------- | -------------------------------------------------------------------- |
| `usb_ep.c`        | Hardware endpoint table, FIFO pairs, primitive read/write            |
| `usb_ctrl.c`      | EP0 control transfer state machine, `usb_ctrl_poll()`               |
| `usb_hid.c`       | Non-blocking HID report buffering and EP flush                       |
| `usb_desc.c`      | All descriptor tables (App); `usb_vendor_desc.c` (BL)               |

---

## 12. Bootloader Architecture (KeyDU.BL)

### 12.1 Entry Conditions

The bootloader runs only when **both** conditions hold at startup:

1. `RSTCTRL.RSTFR` has `SWRF` set — reset was software-triggered via `RSTCTRL.SWRR`.
2. EEPROM byte `0x00` equals `BOOT_MAGIC` — written by the `SYS_BOOT` handler in `keyboard.c` before issuing the software reset.

Power-on reset (`PORF`) and external RESET-pin reset (`EXTRF`) jump straight to the application. `RSTFR` is read and cleared before any peripheral init — flags accumulate across resets, so a stale `SWRF` would otherwise survive into a later power-on reset. The EEPROM magic is also cleared on entry so that subsequent resets start clean regardless of which path is taken.

### 12.2 IVSEL Ordering — Load-Bearing Constraint

The bootloader runs from the boot section and must point interrupt vectors to its own table at `0x0000` (`CPUINT.CTRLA.IVSEL = 1`). The ordering is critical:

- `clock_init()` — called first inside `usb_vendor_init()` — writes `CPUINT.CTRLA = 0` as its last line, clearing `IVSEL`.
- `IVSEL = 1` is therefore set **inside `usb_vendor_init()`, after `clock_init()` and before `usb_init()` / `sei()`**.

Setting `IVSEL` before `usb_vendor_init()` (e.g. in `bl_main.c`) would be silently clobbered by `clock_init()`. With `IVSEL = 0`, USB bus-event interrupts dispatch to the app's vector table at `0x2000`; the BL's `USB0_BUSEVENT_vect` never fires; `usb_device_state` stays `UNATTACHED`; EP0 NAKs every packet (dmesg: `device descriptor read/64, error -110`).

The App is unaffected — it wants `IVSEL = 0` and never sets it; `clock_init()`'s write is the correct final state for the App. Any future boot-section code (e.g. DFU migration) must follow the same ordering.

### 12.3 Vendor Flash Protocol

Five EP0 control-transfer commands (bRequest), defined in `usb_vendor.h`:

| Command       | Code   | Arguments                                          | Effect                                             |
| ------------- | ------ | -------------------------------------------------- | -------------------------------------------------- |
| `VCMD_ERASE`  | `0x01` | `wValue` = page-count code, `wIndex` = addr lo16  | Erase N pages starting at address                 |
| `VCMD_WRITE`  | `0x02` | `wValue` = addr hi16, `wIndex` = addr lo16        | Write 64-byte chunk; auto-commits at page boundary |
| `VCMD_READ`   | `0x03` | `wValue` = addr hi16, `wIndex` = addr lo16        | Read 64-byte chunk                                 |
| `VCMD_RESET`  | `0x04` | none                                               | Jump to application after status ACK               |
| `VCMD_STATUS` | `0x05` | none                                               | Return 1-byte status (`0x00` OK, `0xFF` ERR)       |

Write flow: host issues `VCMD_ERASE` first, then 8 × `VCMD_WRITE` chunks (64 bytes each) to fill a 512-byte page. The BL commits each full page to flash automatically on the 8th chunk. Addresses outside `[APP_START, FLASH_END)` are rejected with `STATUS_ERROR`.

Host tool: `keydu_flasher.c` in `host usb flashing/` (LibUSB 1.0, Linux).

### 12.4 Phase 2 — DFU Migration (Planned)

The vendor-class bootloader is functional but requires a custom host tool. A future phase 2 will migrate to standard DFU (USB class 0xFE / `dfu-util`). The DFU BL is estimated at ~3.8 KB flash, fitting within the current 8 KB boot section. This is deferred until the application firmware is fully stable — see "Keep for phase 2" in `To Dos.md`.

---

## 13. LED / PWM

### 13.1 TCA0 Configuration

Split mode, driving both indicator LEDs independently. See `tca0_pin_info` for pin-to-WO mapping per PORTMUX setting.

### 13.2 Brightness

- Default brightness stored in flash (compiled constant)
- Runtime brightness in SRAM
- `LD_BRIU` / `LD_BRID` on Layer 2 adjust runtime brightness by step
- Range clamped: minimum ~5–10% (if not completely off), maximum 100%
- Brightness step: 16 (out of 255)
- Layer delta: +64 per active layer above 0 (visual feedback of active layer)

---

## 14. Reset and Bootloader

### 14.1 Reset Methods

| Method                    | Mechanism           | Enters BL? |
| ------------------------- | ------------------- | ---------- |
| Hardware tact switch      | RESET pin assertion | No         |
| `SYS_BOOT` key (Layer 2)  | soft reset + magic  | Yes        |
| `SYS_RESET` key (Layer 2) | wdt reset, no magic | No         |
| Power-on reset            | —                   | No         |
| Brown-out reset           | —                   | No         |

### 14.2 EEPROM Magic Mechanism

Refer to bl_main.c, app_main.c and bootmagic.h

### 14.3 Flash Memory Layout

```
0x0000  ┌─────────────────────┐
        │  Bootloader          │  KeyDU.BL — vendor USB class
        │  (8 KB reserved)     │  ~2–6 KB actual, 8 KB reserved
0x2000  ├─────────────────────┤
        │  Application         │  Citrouille90 keyboard firmware KeyDU.App 
        │  (56 KB available)   │
0xFFFF  └─────────────────────┘
```

BOOTSIZE fuse reserves bootloader section in 512-byte increments. Set to 8kb, so 0x10 for 16 pagesof 512B each. UPDI always enabled — never disable in fuses.

Application firmware start address: `0x2000`. Bootloader jumps to `0x2000` when no boot flag is present.

---

## 15. Memory Strategy

### 15.1 Runtime Strategy

| Tier   | Where           | Contents                                                                                                                           |
| ------ | --------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| Flash  | `const PROGMEM` | Keymaps (all 3 layers), macro sequences, USB descriptors, default LED brightness                                                   |
| SRAM   | Global/static   | Active layer index, matrix state, debounce counters, macro execution state, runtime LED brightness, HID report buffers             |
| EEPROM |                 | Magic byte for bootloader flag located at EEPROM byte 0x00. The remaining 255 bytes (offsets 0x01 to 0xFF are free for future use) |

On AVR Dx/DU, flash is memory-mapped — `const` data can be read like normal data with no special flash access API needed on reads. Check alternative approach to access progmem documented in DxCore.

### 15.2 Deferred Tiers

| Tier     | Size  | Intended use                                                                     |
| -------- | ----- | -------------------------------------------------------------------------------- |
| User Row | 64 B  | Unit identity, permanent calibration, bootloader flags. Survives chip erase.     |
| Boot Row | 512 B | Bootloader-private config, firmware validation anchor. App code cannot touch it. |

---

## 16. Scan Loop Timing and main.c

### 16.1 TCB0 Gate in main.c

The main loop is gated on a TCB0 1 ms tick flag — not a busy-poll, not a `delay_ms()`.

The three ISR responsibilities are:

- `TCB0_INT_vect`: set `tick_flag`
- `USB_SOF_vect`: currently empty — reserved for future TCB0/SOF phase-lock
- `USB_BUSEVNT_vect`: handle reset → re-enumerate, suspend → optionally drop to a deeper sleep mode later, resume → restore
- `USB_TRNCOMPL_vect`: handle control EP0 transfers (GET_DESCRIPTOR, SET_REPORT for LED output, SET_IDLE, etc.)

## 17. Build System

### 17.1 Toolchain

- Compiler: `avr-gcc`
- Linker: `avr-gcc` with custom linker script
- Flash tool (UPDI): `avrdude` with `serialupdi` programmer
- Flash tool (USB): custom C + LibUSB (KeyDU.Flasher host tool)

### 17.2 Fuse Configuration (key items)

- `BOOTSIZE`: set to cover actual bootloader size rounded up to 512-byte boundary (reserve 8 KB → BOOTSIZE = 16 = 0x10)
- `UPDI`: never disable — critical for recovery
- Clock: internal 24 MHz oscillator, no external crystal

## 18. Open Items

### 18.1 Remaining verification

1. **Verify LED output report** — confirm Caps Lock/Num Lock LED state arrives via `SET_REPORT` and EP1 OUT. Test on real Citrouille90 PCB (CNano LED on PF2 conflicts with col 2).
2. **Verify consumer report** — confirm media keys (CC_MPLY, CC_VOLU, etc.) reach the host on Interface 1.

*Keyboard reports (basic keypresses, modifiers, 6KRO rollover) confirmed working. Bootloader entry via SYS_BOOT confirmed working.*

### 18.2 Near-term

1. **SOF phase lock** — TCB0 fires 800 µs after prior SOF (200 µs before next); scan completes; report is in queue by SOF. See §11.6 for design. USB stack and basic reporting are stable — this is ready to attempt.
2. **Extend consumer/system keycodes** — see To Dos item 9 for full details.

### 18.3 Deferred (Post-v1)

- USB multipacket for bootloader upload speed
- Boot mouse interface (EP4 IN, if ever)
- NKRO (not planned — 6KRO is sufficient)

## 19 Adapting to future changes

If adopting a different matrix layount (same MCU), adapt:

- `matrix.c` — `COL_F_MASK`, `COL_D_MASK`, `ROW_D_MASK`, `ROW_A_MASK`, `col_pins[]`, `row_a_lut[]`, `scan_col_raw()`
- `matrix.h` — `MATRIX_COLS`, `MATRIX_ROWS`, `DEBOUNCE_TICKS` (if desired)
- `keymap.c` — `KEYMAP()` macro body (physical→electrical remap), keymap arrays
- `keyboard.c` — no changes likely, but verify `PRESSED_KEY_MAX` is still sufficient

If moving to an AVR-DU with more than 32 pins, adapt:

- `matrix.c` — port/pin assignments, masks, `row_a_lut[]`, `scan_col_raw()` (new ports available)
- `encoder.c` — `ENC_A_PORT/VPORT/PIN`, `ENC_B_PORT/VPORT/PIN`
- `led.c` / `led.h` — `LED_PORT`, `LED_A_PIN`, `LED_B_PIN`, PORTMUX routing if TCA0 moves
- `gpio.h` — `GPIO_VPORT_READ` note: verify VPORT availability on new pincount variant
- `fuses.c` — verify fuse constants still match new device header

If moving to an AVR-DU with more flash or more SRAM, adapt:

- `fuses.c` — `BOOTSIZE`, `CODESIZE` (flash partition sizes)
- `flash.h` / `flash.c` — `APP_START`, `FLMAP_SECTION1_ADDR`, `flmap_set()` (FLMAP logic changes if flash exceeds 64KB or sections shift; more sections may be needed)
- `usb_vendor.h` — `APP_START`, `PAGE_SIZE` if the new device differs
- `bl_main.c` — `APP_START` in the jump address calculation
- `usb_vendor.c` — `addr_valid()` upper bound if `FLASH_END` changes
- `makefile` — `MCU`, `F_CPU` if changed, and potentially `flash_*` targets if avrdude flags differ

If you want KeyDU.BL (bootloader) to be larger than 8KB (say 10KB). It's a partition boundary shift:

**Flash partition and linker**

- `fuses.c` — `BOOTSIZE` value (0x10 → 0x14, i.e. 20 × 512B pages)

---

**Shared partition boundary constant**

- `usb_vendor.h` — `APP_START` (0x2000 → 0x2800)
- `avrducore/bootmagic.h` — no change needed, but worth auditing since both sides include it

---

**Bootloader jump target**

- `bl_main.c` — `APP_START` in `jump_to_application()` (picks it up from `usb_vendor.h` indirectly via the include chain, so if `APP_START` is updated there this may be free — verify the include path)

---

**Address validation**

- `flash.h` / `flash.c` — `APP_START` guard in `flash_erase_pages()` and `flash_write_page()` (both use `APP_START` from `usb_vendor.h`, so again may be free if that one constant is updated cleanly)

---

The key insight: `APP_START` in `usb_vendor.h` is the single source of truth for the partition boundary at runtime. The linker scripts and the `BOOTSIZE` fuse are the compile/program-time counterparts. All three must be kept in sync manually — there's no mechanism that enforces agreement between them.
