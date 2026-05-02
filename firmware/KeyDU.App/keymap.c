/* keymap.c — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Owns:
 *   - PROGMEM keymap array (3 layers, 10 rows × 9 cols, electrical layout)
 *   - keymap_key_to_keycode()  layer-aware lookup with transparent fallthrough
 *   - encoder_step()           Alt-Tab / Alt-Shift-Tab on CW / CCW detent
 *   - keymap_tick()            Alt-release timeout, call once per 1 kHz tick
 *
 * Report access
 * -------------
 * encoder_step() manipulates the shared keyboard report exclusively through
 * the kbd_*() API declared in keyboard.h.  It does not access any report
 * struct directly.  This resolves bug #13 (encoder bypassing the seqlock).
 */

#include <avr/pgmspace.h>

#include "keymap.h"
#include "keycode.h"
#include "encoder.h"
#include "keyboard.h"   /* kbd_set_mod(), kbd_clear_mod(), kbd_add_key(),
                           kbd_remove_key(), kbd_stage()                  */
#include "matrix.h"     /* MATRIX_ROWS, MATRIX_COLS                       */

/* ============================================================================
 * KEYMAP() physical→electrical remap macro
 *
 * Physical grid: 20 col × 5 row  →  kNM where N=row (0–4), M=col (0–J hex)
 * Electrical:     9 col × 10 row
 *
 * TODO: replace the body with actual wiring once PCB is confirmed.
 * ========================================================================= */
#define KEYMAP( \
    k00, k01, k02, k03, k04, k05, k06, k07, k08, k09, k0A, k0B, k0C, k0D, k0E, k0F, k0G, k0H, k0I, k0J, \
    k10, k11, k12, k13, k14, k15, k16, k17, k18, k19, k1A, k1B, k1C, k1D, k1E, k1F, k1G, k1H, k1I, k1J, \
    k20, k21, k22, k23, k24, k25, k26, k27, k28, k29, k2A, k2B, k2C, k2D, k2E, k2F, k2G, k2H, k2I, k2J, \
    k30, k31, k32, k33, k34, k35, k36, k37, k38, k39, k3A, k3B, k3C, k3D, k3E, k3F, k3G, k3H, k3I, k3J, \
    k40, k41, k42, k43, k44, k45, k46, k47, k48, k49, k4A, k4B, k4C, k4D, k4E, k4F, k4G, k4H, k4I, k4J  \
) { \
    { k00, k01, k02, k03, k04, k05, k06, k07, k08 }, /* elec row 0 */ \
    { k09, k0A, k0B, k0C, k0D, k0E, k0F, k0G, k0H }, /* elec row 1 */ \
    { k0I, k0J, k10, k11, k12, k13, k14, k15, k16 }, /* elec row 2 */ \
    { k17, k18, k19, k1A, k1B, k1C, k1D, k1E, k1F }, /* elec row 3 */ \
    { k1G, k1H, k1I, k1J, k20, k21, k22, k23, k24 }, /* elec row 4 */ \
    { k25, k26, k27, k28, k29, k2A, k2B, k2C, k2D }, /* elec row 5 */ \
    { k2E, k2F, k2G, k2H, k2I, k2J, k30, k31, k32 }, /* elec row 6 */ \
    { k33, k34, k35, k36, k37, k38, k39, k3A, k3B }, /* elec row 7 */ \
    { k3C, k3D, k3E, k3F, k3G, k3H, k3I, k3J, k40 }, /* elec row 8 */ \
    { k41, k42, k43, k44, k45, k46, k47, k48, k49 }  /* elec row 9 */ \
}

#define NUM_LAYERS  3

/* ============================================================================
 * Keymap — 3 layers, stored in flash
 * ========================================================================= */
const uint16_t PROGMEM keymaps[NUM_LAYERS][MATRIX_ROWS][MATRIX_COLS] = {

    /* ------------------------------------------------------------------
     * Layer 0 — Base
     * ------------------------------------------------------------------ */
    [0] = {
        /* Row 0 */  { KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8    },
        /* Row 1 */  { KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I    },
        /* Row 2 */  { KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K    },
        /* Row 3 */  { KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM },
        /* Row 4 */  { KC_LCTL, KC_LGUI, KC_LALT, KC_SPC,  XXXXXXX, XXXXXXX, KC_RALT, LY_MO1,  KC_RCTL },
        /* Row 5 */  { KC_GRV,  KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, KC_LBRC, KC_RBRC, KC_BSLS },
        /* Row 6 */  { KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS, KC_DEL,  KC_PGUP, KC_PGDN, KC_HOME },
        /* Row 7 */  { KC_L,    KC_SCLN, KC_QUOT, KC_ENT,  KC_END,  KC_UP,   KC_DOWN, KC_LEFT, KC_RGHT },
        /* Row 8 */  { KC_DOT,  KC_SLSH, KC_RSFT, KC_INS,  KC_PSCR, KC_SLCK, KC_PAUS, XXXXXXX, XXXXXXX },
        /* Row 9 */  { KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9   },
    },

    /* ------------------------------------------------------------------
     * Layer 1 — Function
     *
     * Invariant: all modifier positions must be KC_TRNS (______) so that
     * modifier+Fn combos work regardless of press order.  Never KC_NO.
     * ------------------------------------------------------------------ */
    [1] = {
        /* Row 0 */  { ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 1 */  { ______,  MC_COPY, MC_PSTE, ______,  CC_VOLU, CC_VOLD, CC_MUTE, ______,  ______ },
        /* Row 2 */  { ______,  MC_SALL, MC_SAVE, ______,  MC_FIND, ______,  ______,  ______,  ______ },
        /* Row 3 */  { ______,  MC_UNDO, MC_CUT,  MC_COPY, MC_PSTE, ______,  ______,  ______,  ______ },
        /* Row 4 */  { ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, ______,  ______,  LY_MO2 },
        /* Row 5 */  { ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 6 */  { ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 7 */  { ______,  ______,  ______,  ______,  ______,  CC_MPLY, CC_MNXT, CC_MPRV, CC_MSTP },
        /* Row 8 */  { ______,  ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX },
        /* Row 9 */  { ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
    },

    /* ------------------------------------------------------------------
     * Layer 2 — System / config
     * ------------------------------------------------------------------ */
    [2] = {
        /* Row 0 */  { SYS_BOOT, SYS_RST, ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 1 */  { ______,   ______,  ______,  ______,  LD_BRIU, LD_BRID, ______,  ______,  ______ },
        /* Row 2 */  { ______,   ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 3 */  { ______,   ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 4 */  { ______,   ______,  ______,  ______,  XXXXXXX, XXXXXXX, ______,  ______,  ______ },
        /* Row 5 */  { ______,   ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 6 */  { ______,   ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 7 */  { ______,   ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______ },
        /* Row 8 */  { ______,   ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX },
        /* Row 9 */  { KC_F10,   KC_F11,  KC_F12,  ______,  ______,  ______,  ______,  ______,  ______ },
    },
};

/* ============================================================================
 * keymap_key_to_keycode — layer-aware lookup with transparent fallthrough
 * ========================================================================= */
uint16_t keymap_key_to_keycode(uint8_t layer, uint8_t row, uint8_t col)
{
    if (layer >= NUM_LAYERS || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return KC_NO;
    }

    uint16_t kc = pgm_read_word(&keymaps[layer][row][col]);

    while (kc == KC_TRNS && layer > 0) {
        layer--;
        kc = pgm_read_word(&keymaps[layer][row][col]);
    }

    return (kc == KC_TRNS) ? KC_NO : kc;
}

/* ============================================================================
 * Alt-Tab encoder behaviour
 *
 * encoder_step() is called by encoder.c on each confirmed detent.
 * keymap_tick()  is called once per 1 kHz tick by keyboard_task().
 *
 * The shared keyboard report is accessed exclusively through kbd_*() so
 * that all writes go through the seqlock in hid_kbd_stage().  This is the
 * fix for bug #13.
 * ========================================================================= */

#define ALT_RELEASE_TIMEOUT_MS  600u

static uint8_t  s_alt_held;
static uint16_t s_alt_idle_ticks;

/* --------------------------------------------------------------------------
 * encoder_step — CW: Alt+Tab forward.  CCW: Alt+Shift+Tab backward.
 *
 * Alt is held across steps and released by keymap_tick() after
 * ALT_RELEASE_TIMEOUT_MS of encoder inactivity.
 * ------------------------------------------------------------------------ */
void encoder_step(int8_t dir)
{
    /* Latch Alt on the first step of a new gesture. */
    if (!s_alt_held) {
        kbd_set_mod(MOD_LALT);
        kbd_stage();
        s_alt_held = 1;
    }

    s_alt_idle_ticks = 0;

    if (dir == ENCODER_CW) {
        /* Forward: tap Tab while Alt is held. */
        kbd_add_key(KC_TAB);
        kbd_stage();
        kbd_remove_key(KC_TAB);
        kbd_stage();

    } else {
        /* Backward: tap Shift+Tab while Alt is held.
         * Assert Shift, tap Tab, clear Shift — Alt stays latched. */
        kbd_set_mod(MOD_LSFT);
        kbd_stage();

        kbd_add_key(KC_TAB);
        kbd_stage();
        kbd_remove_key(KC_TAB);

        kbd_clear_mod(MOD_LSFT);
        kbd_stage();
    }
}

/* --------------------------------------------------------------------------
 * keymap_tick — call once per 1 kHz tick, after encoder_scan().
 * Releases Alt when encoder goes idle for ALT_RELEASE_TIMEOUT_MS.
 * ------------------------------------------------------------------------ */
void keymap_tick(void)
{
    if (!s_alt_held) {
        return;
    }

    s_alt_idle_ticks++;

    if (s_alt_idle_ticks >= ALT_RELEASE_TIMEOUT_MS) {
        kbd_clear_mod(MOD_LALT);
        kbd_stage();
        s_alt_held       = 0;
        s_alt_idle_ticks = 0;
    }
}
