# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Bare-metal AVR64DU32 USB keyboard firmware. Two build targets sharing common peripheral and USB libraries. No RTOS, no framework, no dynamic allocation.

---

## Repository layout

```
.
├── CLAUDE.md
├── firmware/
│   ├── makefile               ← single master makefile for both targets
│   ├── .astylerc              ← astyle formatting config (--project picks this up)
│   ├── .clangd                ← clangd flags (strips AVR-specific flags, adds include paths)
│   ├── KeyDU.App/             ← HID keyboard application
│   │   ├── app_main.c         ← entry point, TCB0 gate loop
│   │   ├── keyboard.c/.h      ← main keyboard task, key processing
│   │   ├── matrix.c/.h        ← matrix scan (column-output-low / row-input-pullup, 4-tick debounce)
│   │   ├── keymap.c/.h        ← PROGMEM keymap, layer lookup, encoder_step()
│   │   ├── keycode.h          ← keycode defines, IS_* predicates (header-only)
│   │   ├── layer.c/.h         ← layer state machine
│   │   ├── macro.c/.h         ← macro dispatch and PROGMEM sequences
│   │   ├── compose.c/.h       ← compose sequences for accented characters
│   │   ├── led.c/.h           ← TCA0 PWM, LED brightness
│   │   ├── encoder.c/.h       ← hardware encoder scan, calls encoder_step()
│   │   ├── usb_hid.c/.h       ← non-blocking HID report buffering, EP mgmt
│   │   └── usb_desc.c/.h      ← USB descriptor tables
│   ├── KeyDU.BL/              ← vendor-class USB bootloader (planned migration to DFU)
│   │   ├── bl_main.c          ← EEPROM magic check, jump logic, BL state machine
│   │   ├── usb_vendor.c/.h    ← vendor-class USB, flash-write protocol
│   │   ├── usb_vendor_desc.c/.h
│   │   └── flash.c/.h         ← SPM flash-write routines
│   ├── usbcore/               ← USB peripheral layer (shared by App and BL)
│   │   ├── usb_ctrl.c/.h      ← USB device state machine, enumeration, EP0
│   │   ├── usb_ep.c/.h        ← hardware endpoint table, FIFO, primitives
│   │   ├── usb_ep_stream.c/.h ← stream helpers (ep_read_*, ep_write_*)
│   │   └── usb_types.h        ← packed/aligned attributes, EP constants
│   ├── avrducore/             ← AVR64DU32 peripheral HAL (shared)
│   │   ├── clock.c/.h         ← 24 MHz PLL init
│   │   ├── fuses.c            ← fuse configuration
│   │   ├── gpio.h             ← GPIO HAL — header-only, all macros
│   │   ├── bootmagic.h        ← EEPROM magic byte address and value
│   │   └── ccp.S/.h           ← CCP-protected register write helper
│   └── build/                 ← generated artefacts (git-ignored)
│       ├── app/               ← KeyDU.App.elf / .hex / .o
│       ├── bl/                ← KeyDU.BL.elf / .hex / .o
│       └── KeyDU.merged.hex   ← combined image for full UPDI flash
├── host usb flashing/
│   └── keydu_flasher.c        ← host-side USB vendor flash tool (libusb-1.0)
└── doc/
    ├── Firmware reference.md
    └── KeyDU_dev_cheatsheet.md
```

---

## Build commands (run from `firmware/`)

```bash
make              # build both targets (default = all)
make app          # KeyDU.App only
make bl           # KeyDU.BL only
make merged       # build + merge into KeyDU.merged.hex
make size         # build both and print section sizes
make clean        # remove build/

make flash_merged # UPDI: erase + program BL + App (initial bring-up / any BL change)
make flash_app    # UPDI: -D, App only, BL untouched (development iteration)
make flash_bl     # UPDI: -D, BL only, App untouched
make flash_usb    # USB vendor protocol: send App to running BL

make bear         # regenerate compile_commands.json (run make clean first)
make format       # astyle all .c/.h files
make help         # full target reference
```

Programmer: `serialupdi` on `/dev/ttyUSB0` (Adafruit UPDI Friend). Adjust `PROG_PORT` in the makefile if the device node differs.

---

## Toolchain

- Compiler: `avr-gcc` with `-Os -std=gnu11 -mmcu=avr64du32 -DF_CPU=24000000UL`
- Also: `avr-objcopy`, `avr-size`, `avrdude`, `srec_cat` (for merged hex)
- Host tool: `gcc` + `libusb-1.0` (for `keydu_flasher`)
- LSP: `bear` generates `compile_commands.json` for clangd / Kate LSP
- Format: `astyle` reads `firmware/.astylerc` via `--project` flag

---

## Coding conventions

### Naming
- Functions and variables: `snake_case`
- Constants and macros: `UPPER_CASE`
- Types: `snake_case_t` (e.g. `keycode_t`, `ep_fifo_t`)
- Enum members: `UPPER_CASE`; enum types: `name_t`
- No PascalCase anywhere

### Module structure
- `.h` is the public contract: types, public prototypes, constants callers need
- `.c` owns all `static` state, ISR bodies, pin/port definitions, internal helpers
- `static inline` for small hot-path helpers with no external callers
- `gpio.h` is header-only (no `.c` counterpart); all GPIO via macros

### Brace style and indent
Run-in braces (`--style=run-in`), 2-space indent. Enforced by astyle.

### Error handling
No return codes from hardware drivers. Init functions configure registers and return `void`. Use `_Static_assert` for compile-time size/range checks.

### Comments
Inline comment on register writes: explain *why*, not *what*. Public function declarations in `.h`: short plain-English description. No Doxygen. Section separators: `/* --- */` block style.

### PROGMEM
Keymaps, macro sequences, USB descriptors, and CC usage table stored in flash. On AVR64DU32 (≤64 KB flash), `const PROGMEM` data is memory-mapped, but `pgm_read_byte/word` are still used for clarity in hot paths.

---

## Hard resource constraints

| Resource    | Budget       | Notes                                                       |
| ----------- | ------------ | ----------------------------------------------------------- |
| Flash (App) | 56 KB        | App starts at `0x2000`; BL occupies `0x0000–0x1FFF`. Current: ~7.6 KB used (~13%), ~48.6 KB free |
| Flash (BL)  | 8 KB         | Must fit in first 8 KB of flash. Current: ~4.5 KB used (~55%), ~3.6 KB free — the tightest budget; planned DFU BL (~3.8 KB) still fits |
| SRAM        | 8 KB total   | Shared: USB ep table, FIFOs, stack. Current high-water: App ~1.4 KB (~17%); BL ~0.75 KB. App and BL never run at once |
| Scan loop   | 1 kHz (1 ms) | TCB0 gate; **nothing in the scan loop may block**          |

Flag any suggestion that could block in the 1 kHz scan loop context. Run `make size` for current figures.

---

## Flash memory layout

```
0x0000  ┌──────────────────────┐
        │  KeyDU.BL  (8 KB)    │  BOOTSIZE fuse = 0x10 (16 × 512B pages)
0x2000  ├──────────────────────┤  ← APP_START
        │  KeyDU.App (56 KB)   │
0xFFFF  └──────────────────────┘
```

`APP_START` in `usb_vendor.h` is the single runtime constant for the partition boundary. The linker scripts (`-Wl,--section-start=.text=0x2000` for App) and the `BOOTSIZE` fuse must be kept in sync with it manually — there is no mechanism that enforces agreement between them.

**Rebalancing the 8 KB / 56 KB split:** the boundary is not fixed at 8 KB — it is just the current `BOOTSIZE` value. The BL is the tighter budget (~55% full); if a future BL (e.g. the DFU migration plus extras) ever outgrew 8 KB, the boot section can be enlarged to the next 512 B multiple by bumping `BOOTSIZE` and moving `APP_START` down in lockstep. The App has ~48 KB free, so it can cede flash cheaply. All four must change together: `BOOTSIZE`/`CODESIZE` in `avrducore/fuses.c`, `APP_START` in `usb_vendor.h`, the App linker `--section-start=.text=`, and the BL linker flash `LENGTH` (see the header comment in `fuses.c`). Nothing enforces agreement — a mismatch silently breaks the jump or the vector relocation.

---

## Keycode system (`keycode.h` — header-only)

Base type: `uint16_t`. Upper byte = category; lower byte = value.

| Prefix  | Range           | Category                     |
| ------- | --------------- | ---------------------------- |
| `KC_`   | `0x0000–0x00FF` | HID keyboard (page 0x07)     |
| `CC_`   | `0x0100–0x01FF` | Consumer control (page 0x0C) |
| `LY_`   | `0x0200–0x02FF` | Layer switching              |
| `MC_`   | `0x0300–0x03FF` | Macros                       |
| `LD_`   | `0x0400–0x04FF` | LED control                  |
| `SYS_`  | `0x0500–0x05FF` | System / firmware            |
| `CP_`   | `0x0600–0x06FF` | Compose sequences            |

`KC_NO = 0x0000`, `KC_TRNS = 0x0001` (transparent, falls through to lower layer — **not** `0xFF`, which is a valid HID keycode). Aliases: `______` = `KC_TRNS`, `XXXXXXX` = `KC_NO`.

Dispatch uses `IS_*` predicate macros (`IS_LAYER_KEY`, `IS_MACRO_KEY`, etc.) with the `0xFF00` mask — never raw inline comparisons.

**Layer modifier rule:** All modifier key positions on layers 1 and 2 must be `KC_TRNS`. Using `KC_NO` on an upper layer silently breaks modifier+function combos (e.g. Alt+F8 stops working if KC_LALT is KC_NO on layer 1).

---

## Architecture

### KeyDU.App (`firmware/KeyDU.App/`)

Entry: `app_main.c` → `clock_init()` → TCB0 (1 ms tick) → `keyboard_init()` → `usb_init()` → main loop.

Main loop: `hid_flush()` ungated every iteration → `usb_ctrl_poll()` ungated → `keyboard_task()` gated on TCB0 1 ms flag. USB is fully interrupt-driven.

| Module          | Owns                                                               |
| --------------- | ------------------------------------------------------------------ |
| `keyboard.c/.h` | `keyboard_task()`, key event dispatch, HID report state, `kbd_*()` public API |
| `matrix.c/.h`   | 10×9 matrix scan at 1 kHz, 4-tick debounce (3 ms)                 |
| `keymap.c/.h`   | PROGMEM keymap arrays `[layer][row][col]`, layer lookup, `encoder_step()` |
| `layer.c/.h`    | Active layer tracking, momentary layer press/release               |
| `encoder.c/.h`  | Quadrature decode via 16-entry LUT, ±4 accumulator, calls `encoder_step()` |
| `led.c/.h`      | TCA0 split-mode PWM for two LEDs, layer brightness feedback        |
| `macro.c/.h`    | PROGMEM macro sequences, `execute_macro()`                         |
| `compose.c/.h`  | Accented character output via dead-key sequences                   |
| `usb_hid.c/.h`  | SPSC queue for keyboard reports, seqlock for consumer, EP1 IN/OUT  |
| `usb_desc.c/.h` | USB descriptor tables (composite: boot keyboard + consumer)        |
| `gpio.h`        | Header-only GPIO HAL — all macros, no `.c` counterpart             |

### KeyDU.BL (`firmware/KeyDU.BL/`)

Entry: `bl_main.c` reads `RSTFR` and EEPROM magic. Enters bootloader only if SWRF (software reset) **and** EEPROM magic match. Otherwise jumps immediately to `0x2000`. Sets `CPUINT.IVSEL` so interrupt vectors stay in BOOT section.

**IVSEL ordering (load-bearing):** `clock_init()`'s last line writes `CPUINT.CTRLA = 0`, which *clears* `IVSEL`. The BL therefore sets `IVSEL=1` inside `usb_vendor_init()` **after** `clock_init()` (not before, in `bl_main.c`). If `IVSEL` is left at 0, USB bus-event interrupts dispatch to the App's vector table at `0x2000` instead of the BL's at `0x0000`; the BL's bus-reset ISR never runs, `usb_device_state` stays `UNATTACHED`, and EP0 NAKs forever (`dmesg`: `device descriptor read/64, error -110`). The App is unaffected — it wants `IVSEL=0`. Any future boot-section code (e.g. DFU migration) must set `IVSEL` after `clock_init()`. See `Done.md` #6.

Bootloader entry from app: `keyboard.c` `SYS_BOOT` handler writes `BOOT_MAGIC` to EEPROM byte `0x00`, then triggers `RSTCTRL.SWRR`. The BL clears the magic on entry before deciding which path to take.

### Shared libraries

- `usbcore/` — USB device stack (EP0 control, endpoint management, stream helpers). Compiled separately into both App and BL. `usb_get_desc()` and `usb_event_*` callbacks are implemented by the caller — `usb_desc.c` for App, `usb_vendor_desc.c` for BL.
- `avrducore/` — AVR64DU32 primitives: clock init, CCP-protected register writes, fuse config, EEPROM boot magic constants.

---

## USB stack architecture

### Endpoint table layout — FIFOEN=1 (critical)

`USB0.CTRLA` has `FIFOEN` set. This prepends a 32-byte hardware FIFO ring buffer **before** the endpoint pairs in the table struct. EP[0] is at `EPPTR+32`, not `EPPTR+0`. `ep_hw_table_t` in `usb_ep.h` models this with a `uint8_t fifo[32]` member at the start. **Do not remove or reorder that field.**

```c
typedef struct USB_PACKED {
    uint8_t       fifo[32];              /* FIFOEN FIFO — must be first */
    USB_EP_PAIR_t endpoints[EP_TABLE_COUNT];
    uint16_t      frame_num;
} ep_hw_table_t;
```

`EP_TABLE_COUNT` is defined per-target via `-DEP_TABLE_COUNT=N` in the makefile (not in `usb_types.h`). Current values: App = 3 (EP0, EP1 kbd, EP2 consumer), BL = 1 (EP0 only). This sizes both `ep_hw_table_t` and the software FIFO array `ep_fifo_pair_t[EP_TABLE_COUNT]`; keeping it tight recovers ~2 KB of SRAM per binary. For v2 App, change to 6.

### EP0 CONTROL completion (FIFOEN=1)

With FIFOEN enabled, transaction-complete signalling for TYPE=CONTROL endpoints comes through `STATUS.TRNCOMPL` in the endpoint table, not always via `INTFLAGSB`. The ISR (`USB0_TRNCOMPL_vect`) latches completion into `usb_ep_trncompl_in` / `usb_ep_trncompl_out` bitmasks; polling code in `ep_in_ready()` / `ep_out_received()` checks both paths.

### Bus-reset ISR

`USB0.INTCTRLB` (which enables `TRNCOMPL` interrupts) is cleared by the hardware bus-reset event. It **must** be re-enabled explicitly inside the bus-reset ISR after `ep_configure()` runs, and `USB0.FIFORP` must be reset to match `USB0.FIFOWP`.

### SOF / EP0 race — resolved

`hid_flush()` calls `ep_select()` which clobbers global EP pointers. Running it inside the SOF ISR while a control transfer is active corrupts the transfer. **Fixed:** `hid_flush()` is called in the main loop, ungated, before `usb_ctrl_poll()`. Do not move it back into the SOF ISR — the race is live during the HID-probe window (the enumeration window when SOF first goes live at SET_CONFIGURATION), and corrupts GET_DESCRIPTOR(REPORT) / SET_IDLE / SET_PROTOCOL, causing multi-second enumeration stalls and EP1 STALLs.

### Module responsibilities

| Module            | Owns                                                                |
| ----------------- | ------------------------------------------------------------------- |
| `usb_ep.c`        | Hardware endpoint table, FIFO pairs, primitive read/write           |
| `usb_ctrl.c`      | EP0 control transfer state machine, `usb_ctrl_poll()`              |
| `usb_hid.c`       | Non-blocking HID report buffering and EP flush                      |
| `usb_desc.c`      | All descriptor tables (App); `usb_vendor_desc.c` (BL)              |

---

## Bootloader magic flag

GPR2 and GPR3 **do not survive a soft reset** on AVR64DU32 (confirmed on hardware). The bootloader magic flag is stored in EEPROM (implemented in `bl_main.c` and `keyboard.c`). Do not suggest GPR-based approaches for cross-reset state.

---

## GPIO notes

- Prefer `VPORT` registers for the hot matrix scan path — single-cycle `IN` instruction. **Exception:** `PORTD.IN` (regular register) must be used for PORTD row reads in `scan_col_raw()` because PORTD doubles as a column port. Reading `VPORTD.IN` immediately after `PORTD.OUTCLR` triggers the same-port PORT→VPORT collision, returning stale data. `VPORTA.IN` is safe because PORTA is never a column.
- Do not access a VPORT register immediately after the corresponding regular PORT register; leave at least one unrelated instruction between init writes and runtime reads (VPORT collision hazard).
- No PORTB on AVR64DU32 (32-pin package).
- Use `PORTA.PINCTRLUPD` (not bare `PINCTRLUPD`) to avoid ORing ISC bits accidentally.

---

## Known toolchain quirk — GCC 14 FLMAP

GCC 14 injects a `.init3` stub from `libavr64du32.a(flmap_init.o)` that conflicts with non-zero flash origins. Both linker scripts suppress it with:

```
/DISCARD/ : { *flmap_init*(.init3) }
```

This is intentional. If a toolchain upgrade breaks linking, verify with:

```bash
avr-nm /usr/lib/avr/lib/avrxmega2/libavr64du32.a | grep flmap
```

---

## ADC (joystick — PA4/PA5, not yet integrated)

- No `ADC_RESSEL_10BIT_gc` on this MCU; use `ADC_MODE_SINGLE_10BIT_gc` in `ADC0.CTRLA`
- Trigger: write `ADC_START_MUXPOS_WRITE_gc` to `ADC0.COMMAND`; auto-starts on MUXPOS write
- Result: `ADC0.RESULT`
- Pin config: use `PORTA.PINCTRLUPD` (not bare `PINCTRLUPD`) to avoid ORing ISC bits

---

## Known bugs (tracked — do not fix silently without discussion)

None at this time.

---

## KeyDU v2 architecture (planned, not started)

Interface map:
- Interface 0: 6KRO boot keyboard (EP1 IN/OUT)
- Interface 1: consumer/system (EP2 IN)
- Interface 2: NKRO keyboard, 17-byte bitmap (EP3 IN)
- Interface 3: boot mouse 3-byte (EP4 IN)
- Interface 4: report mouse 5-byte + scroll (EP5 IN)
- Interface 5: passive DFU runtime (EP0 only)

`HID_IFACE_COUNT = 5` (interfaces 0–4 only; DFU excluded to prevent HID class requests falling through). `IFACE_TOTAL = 6`; set `-DEP_TABLE_COUNT=6` for App in the makefile, `-DEP_TABLE_COUNT=1` for BL (DFU is EP0-only).

Bootloader will evolve to standard DFU (`dfu-util`, DFU_DNLOAD + DFU_UPLOAD), targeting ~3,800 B flash / ~942 B SRAM.

---

## Hardware reference

- MCU: AVR64DU32 TQFP-32, 64 KB flash, 8 KB SRAM, 24 MHz PLL
- USB connector: Molex 0548190519 (USB Mini-B)
- ESD protection: USBLC6-2SC6
- Encoder: Bourns PES12-42S-N0024 (shaftless 6 mm slotted hub — not D-shaft)
- LEDs: forward voltage 2.1 V (empirically measured; resistor values in `doc/Hardware_notebook.md`)
- Joystick: Top-UP JT8P-3.2T on PA4/PA5 (ADC designed, not integrated)
- Tactile switch: APEM MJTP1230

---

## dmesg USB debug reference

| dmesg message                          | Meaning                                                         |
| -------------------------------------- | --------------------------------------------------------------- |
| No dmesg at all                        | D+ never pulled high — USB_ATTACH_bm not set, clock/VREG issue |
| `device descriptor read/8, error -71`  | Device not responding — USB peripheral not initialised          |
| `device not accepting address`         | EP0 not responding — descriptor or stack issue                  |
| `device descriptor read/64, error -32` | Device reset mid-transfer                                       |
| `error -110` / `error -62`             | Earlier stage failures (pre-enumeration)                        |
| `error -71` / EPROTO                   | Device responds but cannot complete multi-packet transfer        |

USB enumeration is currently working. This table is kept for regression reference.

---

## Development environment

- OS: KUbuntu LTS
- Editor: Kate (KDE) with LSP via clangd + `compile_commands.json`
- Programmer: Adafruit AVR UPDI Friend (`/dev/ttyUSB0`)
- Dev board: Microchip Curiosity Nano EV59F82A (early bring-up / register probing)
- Build: `make` from `firmware/`; parallelised with `-j$(nproc --ignore=2)`

---

## Curiosity Nano test bench

The CNano is mounted on a breadboard using the solderless headers that ship with it.
Flash by dragging `firmware/build/KeyDU.merged.hex` onto the CNano mass-storage USB drive —
never use `make flash_app` (that targets the UPDI wired programmer on `/dev/ttyUSB0`).

### Breadboard pin map

```
Left side  L1–L19 (top→bottom):
  L1  PF1      L2  PF0      L3  PF3      L4  PC3      L5  GND
  L6  PD5      L7  PD4      L8  PD3      L9  PD2      L10 PD1
  L11 PD0      L12 PF5      L13 PF4      L14 VTARGET  L15 GND
  L16 DBG0     L17 DBG3     L18 VOFF     L19 VBUS

Right side  R1–R19 (top→bottom):
  R1  PF2      R2  PF6      R3  PD7      R4  PD6      R5  GND
  R6  PA7      R7  PA6      R8  PA5      R9  PA4      R10 PA3
  R11 PA2      R12 PA1      R13 PA0      R14 DBG2     R15 DBG1
  R16 CDC TX   R17 CDC RX   R18 ID       R19 NC
```

### CNano pin restrictions

Three MCU pins are not freely usable on this board:

| Pin | Breadboard | Reason | Impact |
|-----|------------|--------|--------|
| PF0 | L2 | 32.768 kHz crystal (XTAL32K1) — edge connector disconnected | Col 0 unreachable |
| PF1 | L1 | 32.768 kHz crystal (XTAL32K2) — edge connector disconnected | Col 1 unreachable |
| PC3 | L4 | USB detect (VBUS target sense) | ENC_B unreachable — encoder untestable on CNano |

PF2 (R1) is the CNano's on-board LED. It is usable as col 2 but pulses faintly during
matrix scan (~0.04 % duty cycle when col 2 is driven low). This is harmless.

### Matrix column accessibility on CNano

| Col | MCU pin | Breadboard | Status |
|-----|---------|------------|--------|
| 0   | PF0     | L2  | ❌ crystal — edge disconnected |
| 1   | PF1     | L1  | ❌ crystal — edge disconnected |
| 2   | PF2     | R1  | ⚠ CNano LED (works, faint glow during scan) |
| 3   | PF3     | L3  | ✓ |
| 4   | PF4     | L13 | ✓ verified |
| 5   | PF5     | L12 | ✓ verified |
| 6   | PD5     | L6  | ✓ |
| 7   | PD6     | R4  | ✓ verified |
| 8   | PD7     | R3  | ✓ |

All row pins (PD0–4, PA0–3, PA6) are freely accessible. Keys whose keycode maps to
col 0 or col 1 cannot be tested on the CNano — test those on the real PCB.

### How to find breadboard pins for a key

1. Find the keycode in `keymap.c` layer 0 — note its physical (row, col) label (e.g. k29).
2. Look up the physical label in the KEYMAP macro to get the electrical (row, col).
3. Map electrical col → MCU pin: cols 0–5 = PF0–PF5; cols 6–8 = PD5–PD7.
4. Map electrical row → MCU pin: rows 0–4 = PD0–PD4; row 5 = PA3; row 6 = PA6;
   row 7 = PA2; row 8 = PA1; row 9 = PA0.
5. Look up both pins in the breadboard map above.
6. Short the column pin to the row pin with a seated jumper wire (hold ≥ 4 ms for
   DEBOUNCE_TICKS=4 to commit).
