/* macro.c — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Macro execution.  All report manipulation goes through the kbd_*() API
 * declared in keyboard.h.  No report struct is accessed directly here.
 * This resolves bug #7 (send_mod_key() writing keys[0] directly).
 *
 * Two macro styles:
 *
 *   Simple  — single modifier + key combo, handled inline in execute_macro()
 *             via send_mod_key().
 *
 *   Sequence — multi-step PROGMEM macro_action_t array, executed by
 *              run_macro_sequence().  Private helper aliases (MD/MU/MT/MW)
 *              are defined locally and #undef'd at the end of the file.
 */

#include <avr/pgmspace.h>

#include "macro.h"
#include "keyboard.h"
#include "compose.h"
#include "keycode.h"


/* ============================================================================
 * Private convenience aliases — undefined at the end of this file
 * ========================================================================= */
#define MD(kc)  MACRO_DOWN(kc)
#define MU(kc)  MACRO_UP(kc)
#define MT(kc)  MACRO_TAP(kc)
#define MW(ms)  MACRO_WAIT(ms)

/* ============================================================================
 * Static helpers
 * ========================================================================= */

/* Press a modifier or basic key into the shared report. */
static void press_key(uint8_t kc)
{
    if (IS_MOD_KEY(kc)) {
        kbd_set_mod(MOD_BIT(kc));
    } else {
        kbd_add_key(kc);
    }
}

/* Release a modifier or basic key from the shared report. */
static void release_key(uint8_t kc)
{
    if (IS_MOD_KEY(kc)) {
        kbd_clear_mod(MOD_BIT(kc));
    } else {
        kbd_remove_key(kc);
    }
}

/* Execute a PROGMEM macro_action_t sequence.
 * Terminates on MACRO_ACTION_END or after MAX_MACRO_STEPS steps.
 * Sends a clean report at the end to release any keys left pressed by
 * a malformed or truncated sequence. 
 * 
 * Phase 2 note — MACRO_ACTION_WAIT:
 *   Currently uses _delay_ms() which busy-waits and may interfere with
 *   USB SOF timing.  In phase 2, replace with a sentinel queue entry
 *   (KBD_QUEUE_WAIT) that hid_flush() counts down — see usb_hid.h.
 *   The macro_action_t struct and MACRO_WAIT() macro are unchanged;
 *   only run_macro_sequence() needs updating.
 */
 
#define MAX_MACRO_STEPS  32u

static void run_macro_sequence(const macro_action_t *sequence)
{
    for (uint8_t i = 0; i < MAX_MACRO_STEPS; i++) {
        macro_action_t action;
        action.type    = pgm_read_byte(&sequence[i].type);
        action.keycode = pgm_read_byte(&sequence[i].keycode);

        switch (action.type) {
            case MACRO_ACTION_DOWN:
                press_key(action.keycode);
                kbd_stage();
                break;

            case MACRO_ACTION_UP:
                release_key(action.keycode);
                kbd_stage();
                break;

            case MACRO_ACTION_TAP:
                tap_key(action.keycode);   /* tap_key() now in keyboard.c */
                break;

            case MACRO_ACTION_WAIT:
                /* Phase 1: busy-wait — acceptable for infrequent macro use.
                 * Phase 2: replace with kbd_stage_wait(action.keycode) which
                 * enqueues a KBD_QUEUE_WAIT sentinel — see usb_hid.h. */
                for (volatile uint8_t w = action.keycode; w; w--) {
                    /* spin ~1 ms per count at 24 MHz — rough but sufficient
                     * for phase 1.  No _delay_ms() dependency needed. */
                    for (volatile uint16_t d = 0; d < 2400u; d++) {}
                }               
                break;

            case MACRO_ACTION_END:
                return;
        }
    }
    /* Fell off the end without MACRO_ACTION_END — should not happen,
     * but release everything defensively. */
    kbd_clear_mod(0xFFu);
    kbd_stage();
}

/* ============================================================================
 * PROGMEM sequence definitions
 * ========================================================================= */

/* Excel: Alt, E, S — Paste Special */
static const macro_action_t PROGMEM macro_excel_paste_special[] = {
    MT(KC_LALT), MT(KC_E), MT(KC_S), MACRO_END
};

/* Excel: Ctrl+Shift+Right — select to end of data */
static const macro_action_t PROGMEM macro_excel_select_right[] = {
    MD(KC_LCTL), MD(KC_LSFT), MT(KC_RGHT), MU(KC_LSFT), MU(KC_LCTL), MACRO_END
};

/* Excel: Ctrl+Shift+Down — select to end of column */
static const macro_action_t PROGMEM macro_excel_select_down[] = {
    MD(KC_LCTL), MD(KC_LSFT), MT(KC_DOWN), MU(KC_LSFT), MU(KC_LCTL), MACRO_END
};

/* ============================================================================
 * execute_macro — called from keyboard.c on MC_* keycode press
 * ========================================================================= */
void execute_macro(uint16_t keycode)
{
    switch (keycode) {

        /* Simple modifier+key macros */
        case MC_COPY:  send_mod_key(MOD_LCTL, KC_C); break;
        case MC_PSTE:  send_mod_key(MOD_LCTL, KC_V); break;
        case MC_CUT:   send_mod_key(MOD_LCTL, KC_X); break;
        case MC_UNDO:  send_mod_key(MOD_LCTL, KC_Z); break;
        case MC_REDO:  send_mod_key(MOD_LCTL, KC_Y); break;
        case MC_SAVE:  send_mod_key(MOD_LCTL, KC_S); break;
        case MC_FIND:  send_mod_key(MOD_LCTL, KC_F); break;
        case MC_SALL:  send_mod_key(MOD_LCTL, KC_A); break;

        /* Sequence macros — uncomment and add keycode.h entries as needed */
        /*
        case MC_XL_PSTSP: run_macro_sequence(macro_excel_paste_special);  break;
        case MC_XL_SELR:  run_macro_sequence(macro_excel_select_right);   break;
        case MC_XL_SELD:  run_macro_sequence(macro_excel_select_down);    break;
        */

        default:
            break;
    }
}

/* ============================================================================
 * Undefine private convenience aliases
 * ========================================================================= */
#undef MD
#undef MU
#undef MT
#undef MW
