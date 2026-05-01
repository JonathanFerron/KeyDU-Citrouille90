/* keymap.c — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Owns:
 *   - PROGMEM keymap array (3 layers, 10 rows × 9 cols, electrical layout)
 *   - keymap_key_to_keycode()  — layer-aware lookup with transparent fallthrough
 *   - encoder_step()           — Alt-Tab / Alt-Shift-Tab on CW / CCW detent
 *   - keymap_tick()            — Alt-release timeout, call once per 1 kHz tick
 *
 * Physical layout is 20 columns × 5 rows (100 positions, 10 unused).
 * Electrical layout is 9 columns × 10 rows (90 keys).
 * The KEYMAP() macro below maps physical positions to electrical (row, col).
 * TODO: replace the placeholder mapping with actual wiring once pin
 *       assignments are finalised.  Until then the raw [row][col] tables are
 *       the authoritative keymap.
 */

#include <avr/pgmspace.h>

#include "keymap.h"
#include "keycode.h"   /* KC_*, CC_*, LY_*, MC_*, LD_*, SYS_*, ______, XXXXXXX */
#include "encoder.h"   /* ENCODER_CW, ENCODER_CCW                               */
#include "usb_hid.h"   /* keyboard_report, add_key_to_report(),
                        * remove_key_from_report(), send_keyboard_report()       */
#include "matrix.h"    /* MATRIX_ROWS, MATRIX_COLS                  */

/* ============================================================================
 * KEYMAP() physical→electrical remap macro
 *
 * Physical grid: 20 col × 5 row  →  kNM where N=row (0–4), M=col (0–J hex)
 * Electrical:     9 col × 10 row
 *
 * TODO: replace the body with actual wiring once PCB is confirmed.
 *       Each { ... } line is one electrical row; each element is the physical
 *       position token that lands on that electrical (row, col).
 * ========================================================================= */
#define KEYMAP( \
    k00, k01, k02, k03, k04, k05, k06, k07, k08, k09, k0A, k0B, k0C, k0D, k0E, k0F, k0G, k0H, k0I, k0J, \
    k10, k11, k12, k13, k14, k15, k16, k17, k18, k19, k1A, k1B, k1C, k1D, k1E, k1F, k1G, k1H, k1I, k1J, \
    k20, k21, k22, k23, k24, k25, k26, k27, k28, k29, k2A, k2B, k2C, k2D, k2E, k2F, k2G, k2H, k2I, k2J, \
    k30, k31, k32, k33, k34, k35, k36, k37, k38, k39, k3A, k3B, k3C, k3D, k3E, k3F, k3G, k3H, k3I, k3J, \
    k40, k41, k42, k43, k44, k45, k46, k47, k48, k49, k4A, k4B, k4C, k4D, k4E, k4F, k4G, k4H, k4I, k4J  \
) { \
    /* TODO: map physical positions to electrical (row, col) — placeholder below */ \
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

#define NUM_LAYERS          3

/* ============================================================================
 * Keymap — 3 layers, stored in flash
 *
 * Layer access:
 *   Layer 0 — base layer    (always active unless a layer key is held)
 *   Layer 1 — function      (LY_MO1 on Layer 0, Row 4 Col 7)
 *   Layer 2 — system/config (LY_MO2 on Layer 1, Row 4 Col 8)
 *
 * Notation:
 *   ______   transparent — fall through to the layer below  (KC_TRNS = 0x0001)
 *   XXXXXXX  explicitly disabled — sends nothing            (KC_NO   = 0x0000)
 * 
 * TODO: Make use of the KEYMAP macro here instead of defining the electrical keymap directly.
 * 
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
     *
     * SYS_BOOT  — reboot into bootloader (KeyDU.BL)
     * SYS_RST   — plain firmware reset
     * LD_BRIU / LD_BRID — LED brightness up / down
     * KC_F10–F12 — complete the F-key row started on Layer 0 Row 9
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
 *
 * Walks from the requested layer down to layer 0.  Returns KC_NO when the
 * key is KC_TRNS on every layer, or when any index is out of range.
 * ========================================================================= */
uint16_t keymap_key_to_keycode(uint8_t layer, uint8_t row, uint8_t col)
{
    if (layer >= NUM_LAYERS || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return KC_NO;
    }

    uint16_t kc = pgm_read_word(&keymaps[layer][row][col]);

    /* Walk down through transparent layers iteratively — no stack cost. */
    while (kc == KC_TRNS && layer > 0) {
        layer--;
        kc = pgm_read_word(&keymaps[layer][row][col]);
    }

    /* Layer 0 transparent → no key */
    return (kc == KC_TRNS) ? KC_NO : kc;
}

/* ============================================================================
 * Alt-Tab encoder behaviour
 *
 * encoder_step() is the callback from encoder.c on each confirmed detent.
 * keymap_tick()  is called once per 1 kHz tick from keyboard_task(), after
 *                encoder_scan().
 *
 * State is private to this translation unit.
 * s_alt_idle_ticks is uint16_t — the 600 ms timeout fits comfortably
 * (max value needed: 600 < 65535).
 * ========================================================================= */

/* Milliseconds of encoder inactivity before Alt is released. */
#define ALT_RELEASE_TIMEOUT_MS  600u

static uint8_t  s_alt_held;        /* non-zero while Alt is latched  */
static uint16_t s_alt_idle_ticks;  /* ticks elapsed since last step  */

/* --------------------------------------------------------------------------
 * encoder_step — called by encoder.c on every confirmed detent.
 *
 * CW  → Alt+Tab            (forward through window switcher)
 * CCW → Alt+Shift+Tab      (backward through window switcher)
 *
 * Alt is pressed on the first step and held until the encoder goes idle for
 * ALT_RELEASE_TIMEOUT_MS.  Subsequent steps while Alt is already held send
 * Tab (± Shift) without re-asserting Alt.
 * ------------------------------------------------------------------------ */
void encoder_step(int8_t dir)
{
    /* Latch Alt on the first step of a new gesture. */
    if (!s_alt_held) {
        keyboard_report.mods |= MOD_LALT;
        send_keyboard_report();
        s_alt_held = 1;
    }

    /* Reset idle countdown on every step. */
    s_alt_idle_ticks = 0;

    if (dir == ENCODER_CW) {
        /* Forward: Tab while Alt is held. */
        add_key_to_report(KC_TAB);
        send_keyboard_report();
        remove_key_from_report(KC_TAB);
        send_keyboard_report();

    } else {
        /* Backward: Shift+Tab while Alt is held.
         * Assert Shift, tap Tab, clear Shift — Alt stays held throughout. */
        keyboard_report.mods |= MOD_LSFT;
        send_keyboard_report();

        add_key_to_report(KC_TAB);
        send_keyboard_report();
        remove_key_from_report(KC_TAB);

        keyboard_report.mods &= ~MOD_LSFT;
        send_keyboard_report();
    }
}

/* --------------------------------------------------------------------------
 * keymap_tick — call once per 1 kHz tick from keyboard_task(), after
 *               encoder_scan().
 *
 * Counts idle ticks and releases Alt when the timeout expires.
 * ------------------------------------------------------------------------ */
void keymap_tick(void)
{
    if (!s_alt_held) {
        return;
    }

    s_alt_idle_ticks++;

    if (s_alt_idle_ticks >= ALT_RELEASE_TIMEOUT_MS) {
        keyboard_report.mods &= ~MOD_LALT;
        send_keyboard_report();
        s_alt_held       = 0;
        s_alt_idle_ticks = 0;
    }
}
