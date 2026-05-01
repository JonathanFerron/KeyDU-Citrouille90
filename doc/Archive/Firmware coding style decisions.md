# Citrouille90 — Firmware Coding Style Decisions

## 1. Naming Conventions

- **Functions and variables:** `snake_case`
- **`#define` constants and macros:** `UPPER_CASE`
- **Types:** `snake_case` with `_t` suffix — e.g. `keycode_t`, `matrix_state_t`
- **Enums:** `typedef enum { ... } name_t;` with `UPPER_CASE` members
- No PascalCase anywhere (rules out MCC-style function names)

---

## 2. Keycode System

- Base type: `uint16_t` throughout — keymaps, lookup functions, report logic
- Upper byte encodes category; lower byte encodes value within range
- Ranges and prefixes (defined in `keycode.h`):

| Prefix | Range         | Category            |
|--------|---------------|---------------------|
| `KC_`  | `0x0000–0x00FF` | HID keyboard (Page 0x07) |
| `CC_`  | `0x0100–0x01FF` | Consumer control (Page 0x0C) |
| `LY_`  | `0x0200–0x02FF` | Layer switching |
| `MC_`  | `0x0300–0x03FF` | Macros |
| `LD_`  | `0x0400–0x04FF` | LED control |
| `SYS_` | `0x0500–0x05FF` | System / firmware |

- `KC_NO = 0x0000` — no event (HID Usage 0x00 is reserved)
- `KC_TRNS = 0x0001` — transparent, falls through to lower layer
  - **Not** `0xFF` — that conflicts with a valid HID keycode (Keyboard Application)
- `______` and `XXXXXXX` are aliases defined in `keycode.h`:
  ```c
  #define ______   KC_TRNS   /* transparent — falls through to lower layer */
  #define XXXXXXX  KC_NO     /* explicitly disabled                        */
  ```
- Each `_BASE` sentinel starts at the first value of its range (e.g. `CC_BASE = 0x0100`). Individual codes within a range start at `_BASE | 0x01`, never `_BASE | 0x00`, so no code ever equals its own range sentinel.
- Dispatch in the key processing loop uses the `IS_*` predicate macros (`IS_CONSUMER_KEY`, `IS_LAYER_KEY`, etc.) and the `0xFF00` mask pattern — no magic comparisons inline.
- `GET_LAYER(kc)` extracts the layer index from `LY_*` codes via `(kc) & 0xFF`.
- `MOD_BIT(kc)` converts a `KC_Lxxx` modifier keycode to its HID report bitmask.

---

## 3. GPIO

- Pin descriptors use `gpio_pin_t` struct with `GPIO_PIN(port, pin_n)` initializer — type-safe, zero runtime overhead.
- Operations use `GPIO_HIGH / GPIO_LOW / GPIO_READ / GPIO_TOGGLE` macros that expand directly to `OUTSET / OUTCLR / IN / OUTTGL` register operations.
- `GPIO_VPORT_READ(vport, pin_n)` for single-cycle VPORT reads in hot paths (matrix scan, encoder scan).
- Bulk pin config uses `GPIO_MULTIPIN_PULLUP` and `GPIO_MULTIPIN_DISABLE_INPUT_BUFFER` — writes `PINCONFIG` once, applies with `PINCTRLSET`.
- No wrapper functions around individual GPIO operations — the macros are the API.
- Port-wide bitmasks (`COL_F_MASK`, `ROW_A_MASK`, etc.) are `#define` in the `.c` file where they are used.

---

## 4. Header / Source Split

- **`.h` is the contract:** type definitions, `extern` declarations (minimized), public function prototypes, `#define` constants that callers need.
- **`.c` is the implementation:** static state, internal helpers, ISR bodies, hardware pin definitions, port masks.
- `static` on all internal functions and all module-level state variables — nothing leaks across translation units unintentionally.
- `static inline` for small hot-path helpers that have no reason to be callable from outside the module (e.g. `scan_col` in `matrix.c`).

---

## 5. Error Handling, State Storage, Comments

- **Error handling:** No return codes from hardware drivers. Init functions configure registers and return. Static asserts (`_Static_assert`) for compile-time size and range checks where useful. If hardware init is wrong, the firmware doesn't work — debug at the register level.
- **Comments:**
  - Register operation lines get a brief inline comment on the *why*, not the *what*
  - Public functions in `.h` get a short plain-English description — no Doxygen
  - Section separators use the `/* --- */` block style from `matrix.c` / `encoder.c`
  - Step-numbered comments in non-trivial state machines (as in `encoder_scan`) are encouraged
  - Hardware rationale (RC filter, pull-up decisions, timing) belongs in the relevant `init` function or at the top of the `.c` file

---

## 6. Module Structure and File Naming

Each peripheral subsystem follows the same pattern:

```
module.h   — public API only (prototypes, types, constants callers need)
module.c   — implementation, state, pin definitions, port masks
```

- `keycode.h` — all keycode `#define`s, `IS_*` predicates, `MOD_BIT`, `GET_LAYER`, `______` / `XXXXXXX` aliases. No `.c` counterpart.
- `keymap.h` — prototypes for `keymap_key_to_keycode()` and `keymap_tick()`
- `keymap.c` — PROGMEM keymap array, lookup with transparent fallthrough, `encoder_step()` implementation, `keymap_tick()` for Alt timeout logic
- `matrix.h` / `matrix.c` — matrix init and scan; `register_key()` callback provided by keyboard layer
- `encoder.h` / `encoder.c` — encoder init and scan; `encoder_step()` callback provided by keymap layer
- `gpio.h` — GPIO HAL (header only, all macros)

---

## 7. Encoder Integration

- `encoder_step(int8_t dir)` is implemented in `keymap.c`, not in a separate file. The encoder module is pure hardware; keymap owns all encoder behavior.
- `keymap_tick()` is called once per 1 kHz tick from `keyboard_task()`, after `encoder_scan()`. It owns the Alt-release timeout counter independently — no dependency on a global `millis()` or interrupt-gated timer.
- Alt-tab state variables (`s_alt_held`, `s_alt_idle_ticks`) are `static` in `keymap.c`.
- `s_alt_idle_ticks` is `uint16_t` — the 600 ms timeout fits in 16 bits at 1 kHz tick rate; `uint32_t` is unnecessary.
- Alt-release timeout: 600 ms (tunable via `ALT_RELEASE_TIMEOUT_MS` in `keymap.c`).

---

## 8. Keymap Array

- Stored in flash as `const uint16_t PROGMEM keymaps[NUM_LAYERS][MATRIX_ROWS][MATRIX_COLS]`
- Indexed `[layer][row][col]` — row-major, matching the scan loop order
- Transparent fallthrough in `keymap_key_to_keycode()` walks down layers recursively (or iteratively); returns `KC_NO` when layer 0 is also transparent
- Bounds check on entry: return `KC_NO` for out-of-range indices
- The physical→electrical `KEYMAP()` remap macro is the right approach for mapping the 20×5 physical layout to the 9×10 electrical matrix — implement it with `uint16_t` types once the actual wiring is finalized

---

## Reference Files (as of design freeze)

| File | Status | Notes |
|------|--------|-------|
| `gpio.h` | ✅ Final | Keep as-is |
| `matrix.c` / `matrix.h` | ✅ Final | Style reference for all peripheral modules |
| `encoder.c` / `encoder.h` | ✅ Final | `encoder_step()` stub to be implemented in `keymap.c` |
| `keycode.h` | ✅ Final (rename from `.c`) | Apply `_BASE | 0x01` fix for `CC_`, `LD_`, `SYS_` ranges |
| `keymap.c` | 🔧 Needs split | Separate `.h` / `.c`; absorb alt-tab block from `keymap_alttab_addition_v2.c` |
| `keymap_alttab_addition_v2.c` | 🔧 Merge into `keymap.c` | Fix `s_alt_idle_ticks` to `uint16_t`; remove scaffolding comment block |
| `compile_time_keymap_matrix_transformation.c` | 📐 Design sketch only | Redo with `uint16_t`, correct `KC_TRNS`, actual wiring when pin assignments finalized |
