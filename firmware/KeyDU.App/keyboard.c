/* keyboard.c — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Owns:
 *   - keyboard_init() / keyboard_task()  subsystem lifecycle
 *   - s_kbd_report / s_con_report        the two live HID report structs
 *   - kbd_*() public API                 report manipulation for all callers
 *   - cc_to_hid_usage()                  CC_* keycode → HID Consumer usage
 *   - process_key_press/release()        key event dispatch
 *   - pressed_keys[]                     press-time keycode locking
 *
 * Report model
 * ------------
 * A single hid_kbd_report_t (s_kbd_report) represents the full current
 * keyboard state.  All callers — keyboard.c itself, keymap.c (encoder),
 * and macro.c — manipulate it exclusively through the kbd_*() API defined
 * in keyboard.h.  kbd_stage() pushes it through hid_kbd_stage() into the
 * seqlock double-buffer; hid_flush() in the SOF ISR sends it to the host.
 * This eliminates both bug #7 (direct keys[0] write in macro.c) and
 * bug #13 (encoder writing keyboard_report.mods directly).
 *
 * Consumer reports follow the same pattern via kbd_consumer_set/clear().
 *
 * Bootloader entry (SYS_BOOT)
 * ---------------------------
 * Writes BOOTLOADER_MAGIC to GPR.GPR2 / GPR.GPR3 (complement), then
 * triggers a 15 ms watchdog reset.  The bootloader checks these registers
 * before C-runtime init.  False-positive probability on POR: ~1/65536.
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>

#include "keyboard.h"
#include "keycode.h"
#include "matrix.h"
#include "layer.h"
#include "keymap.h"
#include "macro.h"
#include "led.h"
#include "encoder.h"
#include "usb_hid.h"
#include "usb_ctrl.h"

/* ============================================================================
 * Internal constants
 * ========================================================================= */

#define BOOTLOADER_MAGIC        0x42u
#define BOOTLOADER_MAGIC_COMPL  ((uint8_t)(~BOOTLOADER_MAGIC))

/* ============================================================================
 * HID report state — private to this module
 * ========================================================================= */

static hid_kbd_report_t      s_kbd_report;   /* current keyboard report      */
static hid_consumer_report_t s_con_report;   /* current consumer report      */

/* ============================================================================
 * Consumer usage lookup table
 *
 * Maps CC_* keycodes (lower byte of the 0x01xx range) to 16-bit HID
 * Consumer page (0x0C) usage values.
 * Index: (cc_keycode & 0x00FF) — 1   (CC_BASE | 0x01 is index 0)
 * ========================================================================= */
typedef struct {
    uint8_t  cc_low;      /* lower byte of CC_* keycode */
    uint16_t hid_usage;   /* HID Consumer page usage    */
} cc_usage_entry_t;

// TODO: consider replacing this by a flat switch / case, after weighing the pros and cons
static const cc_usage_entry_t cc_usage_table[] = {
    { 0x01, 0x00E2 },   /* CC_MUTE  — Mute              */
    { 0x02, 0x00E9 },   /* CC_VOLU  — Volume Increment  */
    { 0x03, 0x00EA },   /* CC_VOLD  — Volume Decrement  */
    { 0x04, 0x00B5 },   /* CC_MNXT  — Scan Next Track   */
    { 0x05, 0x00B6 },   /* CC_MPRV  — Scan Prev Track   */
    { 0x06, 0x00CD },   /* CC_MPLY  — Play/Pause        */
    { 0x07, 0x00B7 },   /* CC_MSTP  — Stop              */
};

#define CC_TABLE_LEN  (sizeof(cc_usage_table) / sizeof(cc_usage_table[0]))

static uint16_t cc_to_hid_usage(uint16_t cc_keycode)
{
    uint8_t cc_low = (uint8_t)(cc_keycode & 0x00FFu);
    for (uint8_t i = 0; i < CC_TABLE_LEN; i++) {
        if (cc_usage_table[i].cc_low == cc_low) {
            return cc_usage_table[i].hid_usage;
        }
    }
    return 0x0000u;   /* unknown — send no usage */
}

/* ============================================================================
 * Pressed-key tracking
 *
 * Keycodes are resolved at press time and stored here so that release
 * events use the correct code regardless of layer changes while held.
 * ========================================================================= */
typedef struct {
    uint8_t  row;
    uint8_t  col;
    uint16_t keycode;
} pressed_key_t;

#define PRESSED_KEY_MAX  (MATRIX_ROWS * MATRIX_COLS)

static pressed_key_t s_pressed_keys[PRESSED_KEY_MAX];
static uint8_t       s_pressed_key_count = 0;

/* ============================================================================
 * Forward declarations
 * ========================================================================= */
static void process_key_press(uint8_t row, uint8_t col);
static void process_key_release(uint8_t row, uint8_t col);
static void track_pressed_key(uint8_t row, uint8_t col, uint16_t keycode);
static void untrack_pressed_key(uint8_t row, uint8_t col, uint16_t *keycode_out);

/* ============================================================================
 * Public report API — keyboard
 * ========================================================================= */

void kbd_set_mod(uint8_t mod_bits)
{
    s_kbd_report.modifier |= mod_bits;
}

void kbd_clear_mod(uint8_t mod_bits)
{
    s_kbd_report.modifier &= (uint8_t)(~mod_bits);
}

void kbd_add_key(uint8_t keycode)
{
    for (uint8_t i = 0; i < 6; i++) {
        if (s_kbd_report.keycode[i] == 0x00u) {
            s_kbd_report.keycode[i] = keycode;
            return;
        }
    }
    /* All 6 slots full — 6KRO limit reached, silently drop. */
}

void kbd_remove_key(uint8_t keycode)
{
    for (uint8_t i = 0; i < 6; i++) {
        if (s_kbd_report.keycode[i] == keycode) {
            s_kbd_report.keycode[i] = 0x00u;
            return;
        }
    }
}

void kbd_stage(void)
{
    hid_kbd_stage(&s_kbd_report);
}

/* ============================================================================
 * Public report API — consumer
 * ========================================================================= */

void kbd_consumer_set(uint16_t cc_keycode)
{
    s_con_report.consumer = cc_to_hid_usage(cc_keycode);
    hid_consumer_stage(&s_con_report);
}

void kbd_consumer_clear(void)
{
    s_con_report.consumer = 0x0000u;
    hid_consumer_stage(&s_con_report);
}

/* ============================================================================
 * keyboard_init
 * ========================================================================= */
void keyboard_init(void)
{
    s_pressed_key_count = 0;
    memset(s_pressed_keys, 0, sizeof(s_pressed_keys));

    /* Zero reports; set consumer report_id which must always be 0x01. */
    memset(&s_kbd_report, 0, sizeof(s_kbd_report));
    memset(&s_con_report, 0, sizeof(s_con_report));
    s_con_report.report_id = 0x01u;

    layer_init();
    matrix_init();
    led_init();
    encoder_init();

    usb_init(USB_OPT_VREG_ENABLE);
}

/* ============================================================================
 * keyboard_task — call once per 1 kHz tick from main()
 * ========================================================================= */
void keyboard_task(void)
{
    /* 1. Scan matrix. */
    matrix_scan();

    /* 2. Dispatch press and release events. */
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (matrix_is_key_pressed(row, col)) {
                process_key_press(row, col);
            } else if (matrix_is_key_released(row, col)) {
                process_key_release(row, col);
            }
        }
    }

    /* 3. Encoder scan (may call encoder_step() → keymap.c). */
    encoder_scan();

    /* 4. Alt-Tab timeout tick. */
    keymap_tick();

    /* 5. LED layer feedback. */
    static uint8_t last_layer = 0xFF;
    if (current_layer != last_layer) {
        led_update_layer(current_layer);
        last_layer = current_layer;
    }
}

/* ============================================================================
 * process_key_press
 * ========================================================================= */
static void process_key_press(uint8_t row, uint8_t col)
{
    uint16_t keycode = keymap_key_to_keycode(current_layer, row, col);

    if (keycode == KC_NO) {
        return;
    }

    /* Layer keys */
    if (IS_LAYER_KEY(keycode)) {
        layer_key_pressed(row, col, GET_LAYER(keycode));
        return;
    }

    /* LED brightness */
    if (IS_LED_KEY(keycode)) {
        switch (keycode) {
            case LD_BRIU: led_step(true,  LED_BRIGHTNESS_STEP); break;
            case LD_BRID: led_step(false, LED_BRIGHTNESS_STEP); break;
            default: break;
        }
        return;
    }

    /* System / firmware */
    if (IS_SYSTEM_KEY(keycode)) {
        switch (keycode) {
            case SYS_RST:
                wdt_enable(WDTO_15MS);  // TODO: consider reducing this to a shorter wait time than 15 ms
                while (1);
                break;
            case SYS_BOOT:
                GPR.GPR2 = BOOTLOADER_MAGIC;
                GPR.GPR3 = BOOTLOADER_MAGIC_COMPL;
                wdt_enable(WDTO_15MS);  // TODO: consider using a software reset here rather than a watchdog reset
                while (1);
                break;
            default:
                break;
        }
        return;
    }

    /* Macros — execute atomically on press, no release handling. */
    if (IS_MACRO_KEY(keycode)) {
        execute_macro(keycode);
        return;
    }

    /* Consumer control */
    if (IS_CONSUMER_KEY(keycode)) {
        kbd_consumer_set(keycode);
        track_pressed_key(row, col, keycode);
        return;
    }

    /* Basic HID keys (including modifiers) */
    if (IS_BASIC_KEY(keycode)) {
        if (IS_MOD_KEY(keycode)) {
            kbd_set_mod(MOD_BIT(keycode));
        } else {
            kbd_add_key((uint8_t)keycode);
        }
        kbd_stage();
        track_pressed_key(row, col, keycode);
        return;
    }
}

/* ============================================================================
 * process_key_release
 * ========================================================================= */
static void process_key_release(uint8_t row, uint8_t col)
{
    layer_key_released(row, col);

    uint16_t keycode = KC_NO;
    untrack_pressed_key(row, col, &keycode);

    if (keycode == KC_NO) {
        return;
    }

    if (IS_CONSUMER_KEY(keycode)) {
        kbd_consumer_clear();
        return;
    }

    if (IS_BASIC_KEY(keycode)) {
        if (IS_MOD_KEY(keycode)) {
            kbd_clear_mod(MOD_BIT(keycode));
        } else {
            kbd_remove_key((uint8_t)keycode);
        }
        kbd_stage();
        return;
    }
}

/* ============================================================================
 * track_pressed_key
 * ========================================================================= */
static void track_pressed_key(uint8_t row, uint8_t col, uint16_t keycode)
{
    if (s_pressed_key_count < PRESSED_KEY_MAX) {
        s_pressed_keys[s_pressed_key_count].row     = row;
        s_pressed_keys[s_pressed_key_count].col     = col;
        s_pressed_keys[s_pressed_key_count].keycode = keycode;
        s_pressed_key_count++;
    }
}

/* ============================================================================
 * untrack_pressed_key — O(1) removal by swap with last entry
 * ========================================================================= */
static void untrack_pressed_key(uint8_t row, uint8_t col, uint16_t *keycode_out)
{
    *keycode_out = KC_NO;

    for (uint8_t i = 0; i < s_pressed_key_count; i++) {
        if (s_pressed_keys[i].row == row && s_pressed_keys[i].col == col) {
            *keycode_out = s_pressed_keys[i].keycode;
            s_pressed_key_count--;
            s_pressed_keys[i] = s_pressed_keys[s_pressed_key_count];
            s_pressed_keys[s_pressed_key_count].row     = 0;
            s_pressed_keys[s_pressed_key_count].col     = 0;
            s_pressed_keys[s_pressed_key_count].keycode = KC_NO;
            return;
        }
    }
}
