# AVR64DU32 GPIO optimization reference

A summary of AVR64DU32 features and idioms to apply when writing bare-metal
firmware for this device, derived from datasheet sections 18.3–18.6.

---

## 1. VPORT — single-cycle pin access

### What it is

The four GPIO ports (PORTA, PORTC, PORTD, PORTF) each have a corresponding
Virtual PORT (VPORTA, VPORTC, VPORTD, VPORTF) mapped into the low I/O space
(0x00–0x1F). This makes four registers accessible via the single-cycle `IN`/`OUT`
AVR instructions instead of the two-cycle `LDS`/`STS` required for normal PORT
registers in extended I/O space.

VPORT exposes only four registers: `DIR`, `OUT`, `IN`, `INTFLAGS`.

### When to use it

| Use case | Why |
|---|---|
| Pin reads in tight polling loops | `IN` is one cycle vs two for `LDS` |
| Single-bit set/clear | `SBI`/`CBI` instructions are available; these are atomic — no interrupt window |
| Row reads in a matrix scan | Read an entire port byte in one instruction |
| Encoder pin sampling | Combine two port reads into two `IN` instructions |
| Debug/timing toggle pins | Tightest possible toggle via `SBI`/`CBI` |

### When NOT to use it

- One-time init (direction, pull-up config) — latency is irrelevant
- Ports without a VPORT mapping — only A, C, D, F on this device
- Peripheral registers (TCA0, USART, USB, etc.) — outside I/O space entirely
- `OUTSET`/`OUTCLR`/`OUTTGL` are already single-operation and safe for output
  drives not requiring `SBI`/`CBI` atomicity

### Collision warning

The datasheet warns: **do not access a VPORT register immediately after
accessing the corresponding regular PORT register.** The single-cycle VPORT
access is faster and a memory collision can result. Ensure at least one
unrelated instruction separates a PORT write (in init) from the first VPORT
read at runtime.

### Recommended macro

```c
/* Read a single pin from its VPORT — single-cycle IN instruction.
 * Returns 0 or 1 (not a bitmask).
 * vport: VPORTA / VPORTC / VPORTD / VPORTF
 * pin_n: pin number 0–7 */
#define GPIO_VPORT_READ(vport, pin_n)  (((vport).IN >> (pin_n)) & 1u)
```

### Encoder example

```c
/* Two single-cycle IN instructions — no struct pointer dereference */
uint8_t curr_ab = (GPIO_VPORT_READ(VPORTA, 7) << 1)
                |  GPIO_VPORT_READ(VPORTC, 3);
```

### Matrix scan example

```c
/* Single IN per port — reads the full byte atomically */
uint8_t rows_d = ~VPORTD.IN & ROW_D_MASK;
uint8_t rows_a = ~VPORTA.IN & ROW_A_MASK;
```

---

## 2. Multi-pin configuration — PINCONFIG / PINCTRLUPD / PINCTRLSET / PINCTRLCLR

### What it is

The PORT module provides four registers that configure `PINnCTRL` for multiple
pins in a single write, instead of touching each `PINnCTRL` register
individually.

| Register | Effect |
|---|---|
| `PORTx.PINCONFIG` | Stage the `PINnCTRL` value to apply (ISC, PULLUPEN, INLVL, INVEN) |
| `PORTx.PINCTRLUPD` | Copy `PINCONFIG` verbatim to each selected `PINnCTRL` |
| `PORTx.PINCTRLSET` | Bitwise OR `PINCONFIG` bits into each selected `PINnCTRL` |
| `PORTx.PINCTRLCLR` | Bitwise AND-NOT `PINCONFIG` bits from each selected `PINnCTRL` |

**Key fact:** `PINCONFIG` is mirrored across all ports. Write it once to any
port and it is immediately available to all ports' `PINCTRLUPD/SET/CLR`.

### Caution with PINCTRLSET / PINCTRLCLR and ISC fields

`PINCTRLSET` performs a bitwise OR, so applying a non-zero ISC field ORs into
whatever ISC is already set — the result may be unexpected. Use `PINCTRLUPD`
when you want to overwrite the entire `PINnCTRL`, or ensure the existing ISC
field is zero before using `PINCTRLSET` with an ISC value.

`PINCTRLCLR` performs a bitwise AND-NOT, with the same caveat for ISC.

When setting only `PULLUPEN` (no ISC bits), `PINCTRLSET` is always safe.

### Recommended macros

```c
/* Enable pull-up on multiple pins of the same port in two writes.
 * 'port' is a PORT_t reference (e.g. PORTA), 'mask' is a pin bitmask. */
#define GPIO_MULTIPIN_PULLUP(port, mask) \
    do { \
        (port).PINCONFIG  = PORT_PULLUPEN_bm; \
        (port).PINCTRLSET = (mask); \
    } while (0)

/* Disable digital input buffer on multiple output pins. */
#define GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(port, mask) \
    do { \
        (port).PINCONFIG  = PORT_ISC_INPUT_DISABLE_gc; \
        (port).PINCTRLSET = (mask); \
    } while (0)
```

### Matrix init example

```c
/* Ten individual GPIO_PULLUP_ENABLE calls replaced by two writes per port */
GPIO_MULTIPIN_PULLUP(PORTA, ROW_A_MASK);   /* PA0–PA3, PA6 */
GPIO_MULTIPIN_PULLUP(PORTD, ROW_D_MASK);   /* PD0–PD4      */

/* Disable input buffers on column output pins (reduces power and noise) */
GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTF, COL_F_MASK);
GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTD, COL_D_MASK);
```

Because `PINCONFIG` is mirrored, the PORTA write in `GPIO_MULTIPIN_PULLUP`
also stages the value for the subsequent PORTD write — no second `PINCONFIG`
write is needed.

---

## 3. Disabling digital input buffers

### Why

The datasheet recommends disabling the digital input buffer on pins that are
not used as digital inputs — specifically output pins and analog-only pins.
This reduces power consumption and can reduce noise coupling.

### How

Set `ISC = INPUT_DISABLE` (0x04) in `PINnCTRL`. While set, bit n of `PORTx.IN`
will not update. Use the multi-pin mechanism for any group of pins:

```c
GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTF, COL_F_MASK);
```

For a single pin, use the single-pin macro:

```c
GPIO_DISABLE_INPUT_BUFFER(some_pin);   /* sets ISC = INPUT_DISABLE */
```

---

## 4. Single source of truth for pin assignments

When a pin is accessed through both a `gpio_pin_t` descriptor (for init) and
directly via VPORT (for the hot scan path), define the port and pin number once
and reference the defines in both places.

```c
/* Define once at top of the translation unit */
#define ENC_A_PORT    PORTA
#define ENC_A_VPORT   VPORTA
#define ENC_A_PIN     7
#define ENC_B_PORT    PORTC
#define ENC_B_VPORT   VPORTC
#define ENC_B_PIN     3

/* gpio_pin_t for init (DIRCLR, PULLUP_DISABLE, etc.) */
static const gpio_pin_t ENC_A = GPIO_PIN(ENC_A_PORT, ENC_A_PIN);
static const gpio_pin_t ENC_B = GPIO_PIN(ENC_B_PORT, ENC_B_PIN);

/* VPORT read in the scan hot path */
uint8_t curr_ab = (GPIO_VPORT_READ(ENC_A_VPORT, ENC_A_PIN) << 1)
                |  GPIO_VPORT_READ(ENC_B_VPORT, ENC_B_PIN);
```

Pin reassignment requires changing only the six `#define` lines.

---

## 5. Quick decision guide

```
Is this a one-time init write (direction, pull-up, ISC)?
  └─ Configuring multiple pins on the same port?
       YES → GPIO_MULTIPIN_PULLUP or GPIO_MULTIPIN_DISABLE_INPUT_BUFFER
       NO  → GPIO_SET_INPUT / GPIO_PULLUP_DISABLE / GPIO_SET_ISC (single pin)

Is this an output drive (set, clear, toggle)?
  └─ GPIO_HIGH / GPIO_LOW / GPIO_TOGGLE  (use OUTSET/OUTCLR/OUTTGL — already atomic)

Is this a pin read?
  └─ In a hot scan loop, or need atomicity for single-bit read?
       YES → GPIO_VPORT_READ(VPORTx, pin_n)  — single-cycle IN
       NO  → GPIO_READ(pin)  — fine for init seeding, one-off checks
```

---

## 6. Port / VPORT availability on AVR64DU32 (32-pin)

| GPIO port | VPORT | Notes |
|---|---|---|
| PORTA | VPORTA | Mixed: PA0–PA3, PA6 rows; PA7 encoder A |
| PORTC | VPORTC | PC3 encoder B |
| PORTD | VPORTD | Mixed: PD0–PD4 rows; PD5–PD7 col outputs |
| PORTF | VPORTF | PF0–PF5 col outputs |

There is no PORTB on this device.
