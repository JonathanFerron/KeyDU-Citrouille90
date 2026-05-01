/* keyboard.c — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Top-level keyboard logic.  Owns:
 *   - keyboard_init()      subsystem bring-up
 *   - keyboard_task()      one 1 kHz scan tick
 *
 * Key event model
 * ---------------
 * matrix_scan() performs debounced scanning and maintains its own
 * current/previous state arrays.  keyboard_task() iterates all (row, col)
 * positions after matrix_scan() and calls matrix_is_key_pressed() /
 * matrix_is_key_released() to detect transitions.  There is no
 * register_key() callback in this model.
 *
 * Keycodes are resolved at press time and stored in pressed_keys[] so
 * that release events send the correct code even if the active layer
 * changes while the key is held.
 *
 * Bootloader entry (SYS_BOOT)
 * ---------------------------
 * Writes BOOTLOADER_MAGIC to GPR.GPR2 and its bitwise complement to
 * GPR.GPR3, then triggers a 15 ms watchdog reset.  The bootloader checks
 * these registers before C-runtime init and stays resident when the pattern
 * matches.  False-positive probability on a power-on reset is ~1/65536.
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>

#include "keyboard.h"
#include "matrix.h"
#include "layer.h"
#include "keymap.h"
#include "keycode.h"
#include "macro.h"
#include "led.h"
#include "encoder.h"
#include "usb_hid.h"
#include "config.h"

/* ============================================================================
 * Internal constants
 * ========================================================================= */

#define MATRIX_KEYS     (MATRIX_ROWS * MATRIX_COLS)

/* Magic written to GPR.GPR2/GPR3 to request bootloader entry on next reset. */
#define BOOTLOADER_MAGIC        0xB0u
#define BOOTLOADER_MAGIC_COMPL  ((uint8_t)(~BOOTLOADER_MAGIC))

/* ============================================================================
 * Pressed-key tracking
 *
 * Records the keycode resolved at press time so release can clear the
 * correct report entry regardless of layer changes while the key is held.
 * ========================================================================= */
typedef struct {
    uint8_t  row;
    uint8_t  col;
    uint16_t keycode;
} pressed_key_t;

static pressed_key_t pressed_keys[MATRIX_KEYS];
static uint8_t       pressed_key_count = 0;

/* ============================================================================
 * Forward declarations
 * ========================================================================= */
static void process_key_press(uint8_t row, uint8_t col);
static void process_key_release(uint8_t row, uint8_t col);
static void track_pressed_key(uint8_t row, uint8_t col, uint16_t keycode);
static void untrack_pressed_key(uint8_t row, uint8_t col, uint16_t *keycode_out);

/* ============================================================================
 * keyboard_init
 * ========================================================================= */
void keyboard_init(void)
{
    pressed_key_count = 0;
    memset(pressed_keys, 0, sizeof(pressed_keys));

    layer_init();
    matrix_init();
    macro_init();
    led_init();
    encoder_init();
    usb_init();
}

/* ============================================================================
 * keyboard_task — call once per 1 kHz tick from main()
 * ========================================================================= */
void keyboard_task(void)
{
    /* --- 1. Scan matrix: updates debounced current/previous state. -------- */
    matrix_scan();

    /* --- 2. Dispatch press and release events from edge detection. -------- */
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (matrix_is_key_pressed(row, col)) {
                process_key_press(row, col);
            } else if (matrix_is_key_released(row, col)) {
                process_key_release(row, col);
            }
        }
    }

    /* --- 3. Encoder scan and Alt-Tab timeout tick. ----------------------- */
    encoder_scan();
    keymap_tick();

    /* --- 4. Update LEDs on layer change. --------------------------------- */
    static uint8_t last_layer = 0xFF;
    if (current_layer != last_layer) {
        led_update_for_layer(current_layer);
        last_layer = current_layer;
    }

    /* --- 5. USB task. ----------------------------------------------------- */
    usb_task();
}

/* ============================================================================
 * process_key_press — dispatch a single press event
 * ========================================================================= */
static void process_key_press(uint8_t row, uint8_t col)
{
    uint16_t keycode = keymap_key_to_keycode(current_layer, row, col);

    if (keycode == KC_NO) {
        return;  /* explicitly disabled or transparent on every layer */
    }

    /* --- Layer keys -------------------------------------------------------- */
    if (IS_LAYER_KEY(keycode)) {
        layer_key_pressed(row, col, GET_LAYER(keycode));
        return;
    }

    /* --- LED brightness ---------------------------------------------------- */
    if (IS_LED_KEY(keycode)) {
        switch (keycode) {
            case LD_BRIU: led_brightness_increase(); break;
            case LD_BRID: led_brightness_decrease(); break;
            default: break;
        }
        return;
    }

    /* --- System / firmware keys -------------------------------------------- */
    if (IS_SYSTEM_KEY(keycode)) {
        switch (keycode) {

            case SYS_RST:
                /* Plain watchdog reset — boots directly into application. */
                wdt_enable(WDTO_15MS);
                while (1);
                break;

            case SYS_BOOT:
                /* Signal bootloader entry via GPR magic, then watchdog reset.
                 * Bootloader reads GPR2/GPR3 before C-runtime init. */
                GPR.GPR2 = BOOTLOADER_MAGIC;
                GPR.GPR3 = BOOTLOADER_MAGIC_COMPL;
                wdt_enable(WDTO_15MS);
                while (1);
                break;

            default:
                break;
        }
        return;
    }

    /* --- Macros ------------------------------------------------------------ */
    if (IS_MACRO_KEY(keycode)) {
        /* Macros execute atomically on press; no release handling needed. */
        execute_macro(keycode);
        return;
    }

    /* --- Consumer control keys --------------------------------------------- */
    if (IS_CONSUMER_KEY(keycode)) {
        send_consumer_report(keycode);
        track_pressed_key(row, col, keycode);
        return;
    }

    /* --- Basic HID keys (including modifiers) ------------------------------ */
    if (IS_BASIC_KEY(keycode)) {
        add_key_to_report(keycode);
        send_keyboard_report();
        track_pressed_key(row, col, keycode);
        return;
    }
}

/* ============================================================================
 * process_key_release — dispatch a single release event
 * ========================================================================= */
static void process_key_release(uint8_t row, uint8_t col)
{
    /* Always notify the layer module — it is a no-op for non-layer keys. */
    layer_key_released(row, col);

    /* Retrieve and remove from pressed-key tracking. */
    uint16_t keycode = KC_NO;
    untrack_pressed_key(row, col, &keycode);

    if (keycode == KC_NO) {
        return;  /* was a macro, layer key, LED key, or system key */
    }

    if (IS_CONSUMER_KEY(keycode)) {
        clear_consumer_report();
        send_consumer_report(KC_NO);
        return;
    }

    if (IS_BASIC_KEY(keycode)) {
        remove_key_from_report(keycode);
        send_keyboard_report();
        return;
    }
}

/* ============================================================================
 * track_pressed_key — append a (row, col, keycode) entry
 * Silently drops if the array is full (should not happen at 6KRO).
 * ========================================================================= */
static void track_pressed_key(uint8_t row, uint8_t col, uint16_t keycode)
{
    if (pressed_key_count < MATRIX_KEYS) {
        pressed_keys[pressed_key_count].row     = row;
        pressed_keys[pressed_key_count].col     = col;
        pressed_keys[pressed_key_count].keycode = keycode;
        pressed_key_count++;
    }
}

/* ============================================================================
 * untrack_pressed_key — remove the entry for (row, col); return its keycode.
 * Sets *keycode_out = KC_NO if not found.
 * Fills the gap with the last entry (O(1), order not significant).
 * ========================================================================= */
static void untrack_pressed_key(uint8_t row, uint8_t col, uint16_t *keycode_out)
{
    *keycode_out = KC_NO;

    for (uint8_t i = 0; i < pressed_key_count; i++) {
        if (pressed_keys[i].row == row && pressed_keys[i].col == col) {
            *keycode_out = pressed_keys[i].keycode;

            pressed_key_count--;
            pressed_keys[i] = pressed_keys[pressed_key_count];
            pressed_keys[pressed_key_count].row     = 0;
            pressed_keys[pressed_key_count].col     = 0;
            pressed_keys[pressed_key_count].keycode = KC_NO;
            return;
        }
    }
}
