# Citrouille90 — Firmware Reference

**Project:** Citrouille90 (AVRDU.Enc.Choc)  
**MCU:** AVR64DU32 (TQFP-32), 64 KB flash, 8 KB SRAM, 24 MHz  
**Status as of writing:** Non-USB modules largely final or near-final. USB stack not yet written. Bootloader at pseudocode/stub stage.  
**Document purpose:** Personal working reference. Consolidates all firmware decisions, resolves prior contradictions, and provides a sequenced build plan.

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
- No PascalCase anywhere (rules out MCC-style names as-is)

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

| Prefix | Range           | Category                     |
| ------ | --------------- | ---------------------------- |
| `KC_`  | `0x0000–0x00FF` | HID keyboard (Page 0x07)     |
| `CC_`  | `0x0100–0x01FF` | Consumer control (Page 0x0C) |
| `LY_`  | `0x0200–0x02FF` | Layer switching              |
| `MC_`  | `0x0300–0x03FF` | Macros                       |
| `LD_`  | `0x0400–0x04FF` | LED control                  |
| `SYS_` | `0x0500–0x05FF` | System / firmware            |

- `KC_NO = 0x0000` — no event (HID Usage 0x00 is reserved/safe)

- `KC_TRNS = 0x0001` — transparent, falls through to lower layer. **Not** `0xFF` — that is a valid HID keycode (Keyboard Application).

- `______` and `XXXXXXX` aliases defined in `keycode.h`:
  
  ```c
  #define ______   KC_TRNS   /* transparent — falls through */
  #define XXXXXXX  KC_NO     /* explicitly disabled         */
  ```

- Each `_BASE` sentinel equals the first value of its range (e.g. `CC_BASE = 0x0100`). Individual codes start at `_BASE | 0x01`, never `_BASE | 0x00`, so no code ever equals its range sentinel.

### 3.3 Dispatch Predicates

Dispatch in the key processing loop uses `IS_*` predicate macros and the `0xFF00` mask — no magic inline comparisons:

```c
#define IS_CONSUMER_KEY(kc)  (((kc) & 0xFF00) == 0x0100)
#define IS_LAYER_KEY(kc)     (((kc) & 0xFF00) == 0x0200)
#define IS_MACRO_KEY(kc)     (((kc) & 0xFF00) == 0x0300)
#define IS_LED_KEY(kc)       (((kc) & 0xFF00) == 0x0400)
#define IS_SYS_KEY(kc)       (((kc) & 0xFF00) == 0x0500)
#define GET_LAYER(kc)        ((kc) & 0x00FF)
#define MOD_BIT(kc)          /* converts KC_Lxxx modifier to HID bitmask */
```

---

## 4. Module Architecture

### 4.1 Compiled Source Files (`firmware/KeyDU.App/` on KeyDU-Citrouille90 git repo)

| File            | Status         | Owns                                                                       |
| --------------- | -------------- | -------------------------------------------------------------------------- |
| `gpio.h`        | ✅ Final        | GPIO HAL — header-only, all macros                                         |
| `matrix.c/.h`   | ✅ Final        | Matrix init and scan; `register_key()` callback to keyboard layer          |
| `encoder.c/.h`  | ✅ Final (HW)   | Encoder init and hardware scan only; `encoder_step()` stub                 |
| `keycode.h`     | ✅ Final*       | All keycode `#define`s, `IS_*` predicates, aliases                         |
| `led.c/.h`      | ✅ Good         | TCA0 PWM init and brightness control                                       |
| `keymap.c/.h`   | 🔧 In progress | PROGMEM keymap array, layer lookup, `encoder_step()` impl, `keymap_tick()` |
| `macros.c/.h`   | 🔧 Stub        | Macro dispatch and PROGMEM sequences                                       |
| `keyboard.c/.h` | 🔧 Stub        | Main keyboard logic, `keyboard_task()`, key processing loop                |
| `main.c`        | 🔧 Stub        | Entry point, system init, TCB0 gate loop                                   |
| `usb_hid.c/.h`  | ❌ Not started  | Non-blocking report buffering, HID report structs, EP management           |
| `usb_core.c/.h` | ❌ Not started  | USB device state machine, enumeration                                      |
| `usb_desc.c/.h` | ❌ Not started  | All USB descriptor tables                                                  |

*`keycode.h` needs the `_BASE | 0x01` fix applied to `CC_`, `LD_`, `SYS_` ranges before first compile against actual keymap.

### 4.2 Bootloader (`firmware/KeyDU.BL/`)

| File              | Status        | Owns                                          |
| ----------------- | ------------- | --------------------------------------------- |
| `main.c`          | 🔧 Pseudocode | Early GPR check, jump logic, BL state machine |
| `usb_vendor.c/.h` | ❌ Not started | Vendor class USB, flash write protocol        |
| `flash.c/.h`      | ❌ Not started | SPM flash write routines                      |
| `linker.ld`       | ❌ Not started | Bootloader-specific linker script             |

### 4.3 Not Compiled (`KeyDU-Examples` separate git repo)

Reference implementations for macros and encoder behaviors. Copy desired functions into `KeyDU.App/macros.c` or `KeyDu.App/encoder.c` and compile from there. No preprocessor flags, no dead code.

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
- **Diode orientation:** COL2ROW (one diode per switch, cathode toward column)

### 6.2 Rationale for 1 kHz

USB FS HID polls every 1 ms — scanning at 1 kHz aligns cleanly with the report deadline. A full 90-key scan completes well under 100 µs at 24 MHz, leaving >90% of the 1 ms window for USB and other tasks. 1 kHz is a deliberate architectural choice for determinism, unlike QMK's "scan as fast as possible" which is a historical artifact.

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
    +1,  /* 11→01 valid CCW  */  /* note: these last two match
    -1,  /* 11→10 valid CW   */     the full-cycle symmetry     */
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
- Physical→electrical remap via `KEYMAP()` macro — implement with `uint16_t` once wiring finalized

### 9.2 Layer Summary

| Layer | Name     | Access                     | LED state                         |
| ----- | -------- | -------------------------- | --------------------------------- |
| 0     | Base     | Default                    | Both at base brightness           |
| 1     | Function | Hold Fn (LY_01) on Layer 0 | LED1 brighter (+64), LED2 at base |
| 2     | System   | Hold LY_02 key on Layer 1  | Both brighter (+64)               |

Layer 2 contains: `SYS_BOOT` (enter bootloader), `SYS_RESET` (plain restart), `LD_BRIU`, `LD_BRID` (brightness adjust). Only one layer active at a time; highest active index wins at key press time.

Press-time keycode locking:
Each key slot stores a locked_kc field (type uint16_t) alongside its debounce state. On press, keymap_key_to_keycode(s_active_layer, r, c) is called once and the result stored. On release, locked_kc is used directly — no fresh lookup. Layer changes after press have no effect on in-flight keys.

Keymap invariant — layer 1 and 2:
All modifier key positions on layers 1 and 2 must be KC_TRNS. This enables both LY_01 → mod → key and mod → LY_01 → key orderings for modifier+function combos (e.g. Alt+F8). Leaving a modifier as KC_NO on an upper layer silently breaks that mod for all layer-1/2 combos. Enforce this as a design rule, not just convention.

**key-press-time layer snapshot** or **per-key keycode locking**: when a key transitions from released → pressed, resolve its keycode immediately and store it alongside the key's "pressed" state. All subsequent logic for that key (including the release event) uses the stored keycode, not a fresh lookup.

**Conceptually:**

```c
typedef struct {
    bool     pressed;
    uint16_t locked_kc;   /* resolved at press time, never changes */
} key_state_t;

static key_state_t key_state[MATRIX_ROWS][MATRIX_COLS];
```

On press:

```c
key_state[r][c].pressed   = true;
key_state[r][c].locked_kc = keymap_key_to_keycode(s_active_layer, r, c);
process_keydown(key_state[r][c].locked_kc);
```

On release:

```c
process_keyup(key_state[r][c].locked_kc);   // always uses the locked code
key_state[r][c].pressed = false;
```

---

it's cleaner because the key state struct becomes the single source of truth.

- `key_state_t` gains a `locked_kc` field. If you already have a debounce counter per key, this is just one more field in an existing struct. At 90 keys × 2 bytes = **180 bytes of SRAM**, well within your 8 KB budget.
- `process_matrix_events()` in `keyboard.c` changes from "look up keycode fresh on each event" to "look up on press, use stored value on release." This is maybe 5–10 lines of logic change.
- `keymap_key_to_keycode()` itself doesn't change at all.
- The layer logic (`s_active_layer`) doesn't change at all.

**What you don't need:** Any concept of "sticky layers," layer locks, or more complex layer stacks. The layer index stays simple; the complexity moves to key state tracking, which is already something you need for debounce anyway.

---

Here's what needs to happen concretely, mapped to your existing architecture:

**1. Extend the per-key state struct** (likely in `keyboard.h` or `matrix.h` depending on where you put it)

Add `locked_kc` alongside whatever debounce state already lives there. One struct, one allocation, no separate arrays.

**2. Modify `process_matrix_events()` in `keyboard.c`**

On press: call `keymap_key_to_keycode(s_active_layer, r, c)`, store result in `locked_kc`, then dispatch.
On release: dispatch using `locked_kc` directly — no fresh lookup.

**3. Keymap rule to document in `keymap.c`**

All modifier positions on layers 1 and 2 must be `KC_TRNS`. Add a comment block above the layer 1 definition stating this explicitly as a design invariant, not just a convention.

**Nothing else changes** — `keymap_key_to_keycode()`, the layer index logic, `s_active_layer`, debounce, all untouched.

---

### 9.3 keymap_tick()

Called once per 1 ms tick from `keyboard_task()`, after `encoder_scan()`. Owns the Alt-release timeout counter independently — no dependency on `system_millis`, no interrupt gate overhead.

```c
/* In keymap.c — static, not exposed */
static bool    s_alt_held       = false;
static uint16_t s_alt_idle_ticks = 0;     /* uint16_t: 600 ms fits in 16 bits at 1 kHz */

#define ALT_RELEASE_TIMEOUT_MS  600       /* ticks == ms at 1 kHz scan rate */
```

Alt-tab session logic: when encoder step fires, Alt is pressed and held. Each subsequent step taps Tab. After `ALT_RELEASE_TIMEOUT_MS` ticks of no encoder activity, `keymap_tick()` releases Alt and resets state.

### 9.4 Integration Point in keyboard.c

```c
void keyboard_task(void) {
    matrix_scan();
    encoder_scan();     /* hardware decode — may call encoder_step() */
    keymap_tick();      /* Alt timeout countdown */
    /* ... key processing, report building ... */
}
```

---

## 10. Macro System

### 10.1 Structure

- Macro sequences stored in PROGMEM in `macros.c`
- Dispatch via `execute_macro(uint16_t mc_keycode)` called from `keyboard.c` key processing loop
- Simple modifier+key macros use `send_mod_key()` helper
- Sequence macros use `run_macro_sequence()` stepping through a PROGMEM `macro_action_t` array

### 10.2 Adding a Simple Macro

```c
/* keycode.h */
#define MC_XL_VBA   (MC_BASE | 0x08)   /* Alt+F8 */

/* macros.c */
case MC_XL_VBA:
    send_mod_key(MOD_LALT, KC_F8);
    break;
```

### 10.3 Adding a Sequence Macro

```c
/* keycode.h */
#define MC_XL_PSTSP  (MC_BASE | 0x09)  /* Alt, E, S */

/* macros.c */
static const macro_action_t PROGMEM macro_excel_paste_special[] = {
    MT(KC_LALT), MT(KC_E), MT(KC_S), MACRO_END
};

case MC_XL_PSTSP:
    run_macro_sequence(macro_excel_paste_special);
    break;
```

### 10.4 Private Aliases

If `macros.c` needs temporary short aliases for `KC_` names that would collide with `keycode.h`, define them locally and `#undef` at the end of the file.

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

Consumer codes to support: Volume Up/Down/Mute, Play/Pause, Stop, Next/Prev Track, Browser Home/Back/Forward/Refresh/Search.

### 11.5 Critical Design Constraint — Non-Blocking send_keyboard_report()

`send_keyboard_report()` **must not block**. It writes to a buffer and returns immediately. The USB peripheral handles the actual transaction autonomously when the host polls.

```c
/* usb_hid.c — intended model */
static hid_keyboard_report_t kb_report_buf;
static volatile uint8_t      kb_report_dirty;

void send_keyboard_report(void) {
    memcpy(&kb_report_buf, &keyboard_report, sizeof(kb_report_buf));
    kb_report_dirty = 1;
    /* signal endpoint — implementation detail resolved when writing USB stack */
}
```

**Why this is non-negotiable:** `encoder_step()` calls `send_keyboard_report()` multiple times inline for the Alt+Tab tap sequence. If blocking, this stalls for several ms mid-scan, breaking the entire 1 kHz timing architecture. Document this constraint explicitly in `usb_hid.h` before writing endpoint code.

You need a true double-buffer or critical section. The weakness of the buffered approach is a **torn-read race condition.** Your scan loop writes an 8-byte report struct; your ISR reads it. On AVR, an 8-byte write is never atomic — it's 8 separate `ST` instructions. If the ISR fires mid-write, the host receives a half-updated report (e.g., modifier byte from the new state, keycodes from the old state).

You have two clean solutions:

- **Double-buffer with a dirty flag.** Scan loop writes to a back buffer and sets a flag. ISR checks the flag, copies the back buffer to the front buffer atomically (or just sends directly from back buffer if you design the flag protocol carefully).
- **`ATOMIC_BLOCK` around the ISR read.** Simpler: the ISR wraps its read of the shared buffer in `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`, which on AVR-DU means briefly masking global interrupts during the copy. Since the report is only 8 bytes, this is on the order of 8–16 cycles — negligible.
  - ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { memcpy(&front_buf, &back_buf, sizeof(report_keyboard_t)); }
  - Global interrupts are disabled for those ~16 cycles. The ISR cannot fire mid-copy. This is safe because 16 cycles at 24MHz is under 1µs — your USB timing and encoder lockout windows are all in the millisecond range, so nothing cares.
    
    The subtlety: you'd call this from the **scan loop** side before setting the dirty flag, not from inside the ISR. The ISR then reads `front_buf` freely, because by the time it runs, the copy is guaranteed complete.

### 11.6 SOF Interrupt and Scan Loop Relationship

`USB_SOF_vect` fires every 1 ms when host is connected and not suspended.

- **Primary role:** flush queued report to host (report was prepared by prior scan): more precisely, this happens automatically on the usb hardware side when the host sends a report in request, as long as the firmware placed a report in the usb buffer
- **Secondary role (future):** snapshot `TCB0.CNT` for phase measurement
- **Not** the scan trigger — scan is triggered by TCB0

TCB0 runs unconditionally. During USB suspend or before enumeration, SOF stops but `keyboard_task()` keeps running from TCB0 regardless. No special handling needed in scan code.

**Correct HID report sequence:** Scan → queue report → send on next host SOF poll. Report reflects the world just before the host asked, not up to 1 ms ago.

### 11.7 Future Optimization — SOF Phase Lock

After USB stack is stable and scan duration is measured on real hardware:

```
TCB0 fires at (SOF_phase - scan_budget):
    → matrix_scan() → encoder_scan() → keymap_tick()
    → report ready in queue

SOF ISR fires ~scan_budget ms later:
    → flush queued report to host
```

Phase measurement: snapshot `TCB0.CNT` in `USB_SOF_vect`, average 8–16 samples. Adjust TCB0 compare register. Measure scan budget with GPIO toggle + logic analyzer (target < 0.2 ms / 4800 counts at 24 MHz).

**Prerequisites before attempting:** USB stack enumerating and reporting reliably; scan duration measured; `send_keyboard_report()` confirmed non-blocking.

As a first guess, just assume a conservative scan budget of say 200 microseconds.

### 11.8 Bootloader Mode — Vendor Class

- Vendor-defined class, single interface
- Bulk transfers for firmware upload
- Host tool: Python, C or avrdude (vendor class driver) + LibUSB
- Reference: `avr64du32-cnano-usb-vendor-mplab-mcc` (Microchip GitHub) — study approach, adapt to bulk transfer style
- USB multipacket transfers: design bootloader OUT endpoint to use these from the start — reduces CPU interrupts per 64KB upload and simplifies flashing tool timing
- Keep in mind max packet size of 64B and page size of 512B

---

## 12. USB Stack Implementation Plan

### 12.2 LUFA RAMPZ Bug

`LUFA/Platform/UC3/ClockManagement.h` (and possibly others) reference `RAMPZ`. The AVR64DU32 does not have `RAMPZ` — the register does not exist on this device. This is a LUFA bug. Fix: `#ifdef`-guard the `RAMPZ` references like the way it's done in the MCC generated code. Confirmed: `RAMPZ` is not required for AVR64DU32 operation.

### 12.3 MCC Code Usage

Trust MCC-generated register sequences for: clock init, TCA0 PWM, USB peripheral init. Do not trust MCC file organization — MCC scatters logic across many small files in deep folder hierarchies. Restructure aggressively into the project's own file layout while keeping the register-level code intact.

### 12.4 What to Take from QMK

- Layer system and keycode architecture patterns (study `quantum/keymap_common.c`)
- HID report building: modifier byte packing, 6KRO key slot management
- `process_keycode/` subdirectory: consumer control dispatch, media key routing
- USB suspend/resume handling approach

**Do not port:** VIA/VIAL, RGB matrix, audio, split keyboard support, NKRO (6KRO is sufficient and boot-protocol-compatible).

### 12.5 What to Take from TinyUSB

- `hid_keyboard_report_t` struct definition (already in TinyUSB reference doc)
- HID modifier bitmask defines
- Vendor class descriptor structure pattern
- Boot protocol handling approach

TinyUSB itself is not used — too much abstraction overhead for a single-target bare-metal build.

---

## 13. LED / PWM

### 13.1 TCA0 Configuration

Split mode, driving both indicator LEDs independently:

```c
/* 1465 Hz PWM: f = 24 MHz / (64 prescaler × 256 period) */
TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc;
```

TCA0 WO2 and WO3 (or WO4/WO5 depending on pin assignment via TCAROUTEA) drive the two PWM LED pins. See `tca0_pin_info` for pin-to-WO mapping per PORTMUX setting.

### 13.2 Brightness

- Default brightness stored in flash (compiled constant)
- Runtime brightness in SRAM
- `LD_BRIU` / `LD_BRID` on Layer 2 adjust runtime brightness by step
- Range clamped: minimum ~5–10% (if not completely off), maximum 100%
- Brightness step: 16 (out of 255)
- Layer delta: +64 per active layer above 0 (visual feedback of active layer)

### 13.3 EEPROM for Brightness Persistence

Deferred — not in v1. Runtime brightness is lost on power cycle. If persistence is added later, EEPROM (256 bytes) is the right place: one byte for brightness level, survives resets but not power-on (exactly the right semantic). User Row (64 bytes, survives chip erase) is for unit identity and permanent calibration, not user preferences.

---

## 14. Reset and Bootloader

### 14.1 Reset Methods

| Method                    | Mechanism            | Enters BL? | GPR preserved? |
| ------------------------- | -------------------- | ---------- | -------------- |
| Hardware tact switch      | RESET pin assertion  | No         | Test on Nano   |
| `SYS_BOOT` key (Layer 2)  | soft reset + magic   | Yes        | Test on Nano   |
| `SYS_RESET` key (Layer 2) | soft reset, no magic | No         | Test on Nano   |
| Power-on reset            | —                    | No         | No             |
| Brown-out reset           | —                    | No         | No             |

### 14.2 GPR Magic Mechanism

```c
#define BOOT_MAGIC  0x42u   /* arbitrary distinctive value */

void request_bootloader(void) {
    /* Write magic to GPR0 before reset fires */
    asm volatile ("ldi r2, %0\n" : : "M" (BOOT_MAGIC));
    /* Optionally also write complement to GPR1 for ~1/65536 false positive rate */
    // Call software reset
}

void software_reset(void) {
    /* No magic — GPR left at whatever it was */
    // Call software reset
}
```

Bootloader startup (at the verybeginning of main()):

```c
if (GPR.GPR0 == BOOT_MAGIC) { // may also check reset flag
    GPR.GPR0 = 0;           /* clear immediately */
    // may possibly store reset flag
    // clear reset flag
    stay_in_bootloader();
} else {
    jump_to_application();
}
```

### 14.3 GPR Retention Test — Pending

**Must test on Curiosity Nano AVR64DU32 before relying on this mechanism:**

- Does GPR survive RESET pin assertion? (ClaudeAI says yes for WDT and soft reset; RESET pin behavior needs confirmation)
- Does GPR survive direct software reset via `_PROTECTED_WRITE(RSTCTRL.SWRR, ...)`?

If GPR does survive hardware RESET pin: the tact switch could cause false bootloader entry if GPR happens to contain `0x42` — low probability (~1/256) but not zero. The dual-register check (GPR0 == 0x42 AND GPR1 == ~0x42) reduces this to ~1/65536. Checking the reset flag eliminates this risk.

### 14.4 Flash Memory Layout

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

### 15.1 Two-Tier Runtime Strategy (v1)

| Tier  | Where           | Contents                                                                                                               |
| ----- | --------------- | ---------------------------------------------------------------------------------------------------------------------- |
| Flash | `const PROGMEM` | Keymaps (all 3 layers), macro sequences, USB descriptors, default LED brightness                                       |
| SRAM  | Global/static   | Active layer index, matrix state, debounce counters, macro execution state, runtime LED brightness, HID report buffers |

On AVR Dx/DU, flash is memory-mapped — `const` data can be read like normal data with no special flash access API needed on reads. Check alternative approach to access progmem documented in DxCore.

### 15.2 Deferred Tiers (post-v1)

| Tier     | Size  | Intended use                                                                     |
| -------- | ----- | -------------------------------------------------------------------------------- |
| EEPROM   | 256 B | User runtime preferences: LED brightness persistence, future profile index       |
| User Row | 64 B  | Unit identity, permanent calibration, bootloader flags. Survives chip erase.     |
| Boot Row | 512 B | Bootloader-private config, firmware validation anchor. App code cannot touch it. |

---

## 16. Scan Loop Timing and main.c

### 16.1 TCB0 Gate in main.c

The main loop is gated on a TCB0 1 ms tick flag — not a busy-poll, not a `delay_ms()`:

```c
/* main.c skeleton */
int main(void) {
    system_init();      /* clock, etc. */
    gpio_init();
    matrix_init();
    encoder_init();
    led_init();
    usb_init();
    tcb0_init();        /* 1 ms tick, sets flag in ISR */
    sei();
    sleep_enable();   /* sets SE bit in SLPCTRL — required before sleep_cpu() works */
    set_sleep_mode(SLEEP_MODE_IDLE); // from <avr/sleep.h>

    while (1) {
        if (tick_flag) {
            tick_flag = 0;
            matrix_scan();
            encoder_scan();         /* may call encoder_step() → keymap_tick indirectly */
            keymap_tick();          /* Alt timeout, other tick-based state */
            process_matrix_events();/* key press/release → keycode dispatch */
            /* build and queue HID reports */
        }
        /* nothing else to do — USB is fully interrupt-driven */
        sleep_cpu();   /* IDLE mode: wakes on any interrupt (TCB0, USB, etc.) */
    }
}
```

`tick_flag` is `volatile uint8_t`, set in `TCB0_INT_vect`, cleared in main loop before `keyboard_task()`.

The USB peripheral on the AVR64DU32 fires distinct interrupt vectors for exactly the events `usb_task()` would otherwise poll:

- `USB_BUSEVNT_vect` — bus reset, suspend, resume, connect/disconnect
- `USB_SOF_vect` — start-of-frame (already in your plan for report flushing)
- `USB_TRNCOMPL_vect` — transaction complete (control transfers, endpoint activity)

Your three ISR responsibilities become:

- `TCB0_INT_vect`: set `tick_flag`
- `USB_SOF_vect`: flush queued report to endpoint buffer (already planned)
- `USB_BUSEVNT_vect`: handle reset → re-enumerate, suspend → optionally drop to a deeper sleep mode later, resume → restore
- `USB_TRNCOMPL_vect`: handle control EP0 transfers (GET_DESCRIPTOR, SET_REPORT for LED output, SET_IDLE, etc.)

### ---

## 17. Build System

### 17.1 Toolchain

- Compiler: `avr-gcc`
- Linker: `avr-gcc` with custom linker script
- Flash tool (UPDI): `avrdude` with `serialupdi` programmer
- Flash tool (USB): custom Python, C or avrdude (with vendor programmer) + LibUSB (KeyDU host tool)

### 17.2 Fuse Configuration (key items)

- `BOOTSIZE`: set to cover actual bootloader size rounded up to 512-byte boundary (reserve 8 KB → BOOTSIZE = 16 = 0x10)
- `UPDI`: never disable — critical for recovery
- Clock: internal 24 MHz oscillator, no external crystal

### 17.3 avrdude Command Pattern

```bash
avrdude -c serialupdi -p avr64du32 -P /dev/ttyUSBx \
        -U flash:w:firmware.hex:i
```

---

## 18. Open Items and Build Sequence

### 18.1 Immediate — Unblock USB

These must happen in order; everything downstream depends on USB working:

1. **Fix `keymap.c`:** Split into proper `.h`/`.c`; merge `keymap_alttab_addition_v2.c` in; fix `s_alt_idle_ticks` to `uint16_t`; apply `_BASE | 0x01` fix to `CC_`, `LD_`, `SYS_` ranges in `keycode.h`.
2. **Strip LUFA EchoRomeo and Vendor demos** down to AVR64DU32-applicable code. Fix `RAMPZ` references. Reorganize into `usb_core.c/.h`, `usb_desc.c/.h`, `usb_hid.c/.h`.
3. **Write `usb_hid.c`** — establish non-blocking `send_keyboard_report()` first, before anything else in that file. Document the non-blocking constraint in `usb_hid.h`.
4. **Write `usb_desc.c`** — composite HID descriptor for 2 interfaces (boot keyboard + extended controls). Include EP1 OUT for LED output report.
5. **Write `usb_core.c`** — device state machine, enumeration, `USB_SOF_vect` ISR (flush queued report; phase measurement stub for later).
6. **Fix `main.c`** scan loop gate (TCB0 tick flag pattern from section 16.1).
7. **Verify enumeration** on Curiosity Nano — confirm device appears as HID composite, check descriptor with USB descriptor viewer tool (e.g. USB Device Tree Viewer on Windows).
8. **Verify keyboard reports** — confirm keystrokes reach host, modifiers work, 6KRO rollover correct.
9. **Verify LED output report** — confirm Caps Lock LED state arrives via `SET_REPORT`.
10. **Verify consumer report** — confirm media keys reach host on Interface 1.

### 18.2 Parallel — GPR Test

Before writing any bootloader code, run the GPR retention test on the Curiosity Nano:

- Write known value to `GPR.GPR0`
- Assert RESET pin (tact switch)
- On startup, read `GPR.GPR0` — retained or cleared?
- Repeat for software reset via `_PROTECTED_WRITE(RSTCTRL.SWRR, ...)`

Results determine whether the dual-register check is needed and whether the hardware tact switch requires the complement guard.

### 18.3 After USB App Firmware is Stable

11. **Measure scan duration** with GPIO toggle + logic analyzer. Confirm < 0.2 ms. Record actual value.
12. **Write `macros.c`** — `execute_macro()` dispatch, initial set of PROGMEM sequences.
13. **Complete `keyboard.c`** — full key processing loop, report building, layer dispatch, `add/remove_key_to_report()`.
14. **Write `firmware/bootloader/`** — vendor USB class, flash write routines, linker script. Port from bootloader pseudocode stubs.
15. **SOF phase lock** — revisit after scan duration measured and USB stack proven stable.

### 18.4 Deferred (Post-v1)

- EEPROM persistence for LED brightness
- USB multipacket for bootloader upload speed
- SOF phase lock optimization
- Boot mouse interface (eg on Interface 2, EP3 IN)
- NKRO (not planned — 6KRO is sufficient)
