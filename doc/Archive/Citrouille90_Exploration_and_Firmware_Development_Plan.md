# Citrouille90 MCC Exploration and Firmware Development Plan

## 

**Core Philosophy:**

- Bare metal, minimalistic approach
- Easily readable and maintainable code
- Internal harmony
- Learn from multiple sources, choose the best ideas intentionally

---



```
## HAL Design Philosophy

- Pin/Port naming (e.g., `LED_A_PIN` vs `INDICATOR_LED_1` vs `pin_led_indicator_a`)
- Macro naming (e.g., `LED_ON()` vs `led_on()` vs `SET_LED_STATE()`)

- Setter pattern (`led_set_brightness(LED_A, 50)` vs `LED_A_brightness = 50`)
- Getter pattern (`led_get_brightness(LED_A)` vs `LED_A.brightness`)
- Toggle/action pattern (`led_toggle(LED_A)` vs `LED_TOGGLE(A)`)
- Multi-parameter functions vs config structs
- Use of enums vs #defines for constants
- Callback/event model (if applicable)
- Initialization parameter passing

**LED HAL Example:**

Pin Definition Strategy:
```c

Preferred:       #define LED_A_PORT PORTA
            #define LED_A_PIN  4


```

Basic Operations:

```c
Preferred:       LED_A_PORT.OUTSET = LED_A_PIN;
                 LED_A_PORT.OUTCLR = LED_A_PIN;


```

PWM Control:

```c
Preferred:       LED_SetDutyCycle(PWM_LED_A, 127);


```

Initialization:

```c
MCC:        LED_Initialize();
            PWM_Initialize();


```

#### ---

### #### HID Report Mapping

How keycodes become USB reports.

**Columns:**

- Mapping Aspect
- MCC
- DxCore
- LUFA
- TinyUSB
- QMK
- ClaudeAI Draft
- My Implementation

**Rows:**

- Basic HID keycode translation
- Modifier handling
- Consumer control translation
- System control translation
- Mouse button mapping
- Multi-interface report routing
- Report buffer management

**Example:**

```
QMK:
  Has keycode_to_system() helper
  Has keycode_to_consumer() helper
  Checks keycode ranges to route to correct interface

  Preferred option: User keymaps use KC_* names
    Pro: Readable, portable
    Con: Need translation layer
```

#### #### Concrete Examples

Work through real scenarios to test the design.

**Example 1: Volume Up Key**

```
Keymap entry:  KC_VOLU  (what user types)
Internal code: 0xE9     (internal representation)
USB interface: Interface 2 (Extended Controls)
HID page:      0x0C (Consumer)
HID usage:     0xE9 (Volume Up)
Report byte:   Send 0x00E9 in bytes 1-2 of Interface 2 report
```

**Example 2: Layer-Tap Key**

```
Keymap entry:  LT(1, KC_SPC)  (tap=space, hold=layer 1)
Internal code: 0x7E01 | (KC_SPC << 8)  (or however encoded)
Processing:    Key event handler checks hold duration
               <threshold: send KC_SPC to Interface 0
               >=threshold: activate layer 1, don't send key
```

**Example 3: Macro Key**

```
Keymap entry:  MACRO_VOL_UP  (custom encoder macro)
Internal code: 0x8000 | MACRO_ID_VOL_UP
Processing:    Macro engine executes sequence
               Sends KC_VOLU to Interface 2
```
