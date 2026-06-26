/* keymap.c — Citrouille90 / AVR64DU32
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   Owns:
     - PROGMEM keymap array (3 layers, 10 rows × 9 cols, electrical layout)
     - keymap_key_to_keycode()  layer-aware lookup with transparent fallthrough
     - encoder_step()           Alt-Tab / Alt-Shift-Tab on CW / CCW detent
     - keymap_tick()            Alt-release timeout, call once per 1 kHz tick

   Report access
   -------------
   encoder_step() manipulates the shared keyboard report exclusively through
   the kbd_*() API declared in keyboard.h.  It does not access any report
   struct directly.  This resolves bug #13 (encoder bypassing the seqlock).
*/

#include <avr/pgmspace.h>

#include "keymap.h"
#include "keycode.h"
#include "encoder.h"
#include "keyboard.h"
#include "matrix.h"

/* ============================================================================
   KEYMAP() — physical→electrical layout mapping

   Physical grid: 5 rows × 19 cols (left→right, 0-indexed).
   Electrical grid: 10 rows × 9 cols (0-indexed in firmware arrays, 1-indexed in KiCad netlist).

   Parameter naming: kRC
     R = physical row  0–4
     C = physical col  0–9, then A–I for cols 10–18

   Physical column layout (left→right):
     Cols  0– 2  left outer    elec col 8–6 (KiCad COL9–7), elec row 0–4
     Cols  3– 5  left mid      elec col 5–3 (KiCad COL6–4), elec row 0–4
     Col   6     squeezed cluster  R3C3/R4C3/R4C2, phys rows 2–4 (rows 0–1 empty)
     Col   7     R1C3, R2C3, R3C2, R4C1, R5C3
     Col   8     R1C2, R2C2, R3C1, R9C1, R5C2
     Col   9     R1C1, R2C1, R8C1, R9C2, R5C1
     Cols 10–12  center-right bridge  (KiCad COL1–3 of ROW6–10 + ROW8–9)
     Col  13     R6C4, R7C4, R8C5, —,    R10C4
     Col  14     R6C5, R7C5, —,    R10C5, R10C6
     Col  15     R6C6–R9C6 (rows 0–3), row 4 empty
     Cols 16–18  right numpad  elec col 6–8 (KiCad COL7–9), elec row 5–9

   5 slots have no electrical connection (XXXXXXX in all layers):
     phys (row0,col6) or k06, (row1,col6) or k16, (row0,col13) or k0D, (row0,col14) or k0E, (row4,col15) or k4F

   ========================================================================= */
#define KEYMAP( \
                k00, k01, k02, k03, k04, k05, k06, k07, k08, k09, k0A, k0B, k0C, k0D, k0E, k0F, k0G, k0H, k0I, \
                k10, k11, k12, k13, k14, k15, k16, k17, k18, k19, k1A, k1B, k1C, k1D, k1E, k1F, k1G, k1H, k1I, \
                k20, k21, k22, k23, k24, k25, k26, k27, k28, k29, k2A, k2B, k2C, k2D, k2E, k2F, k2G, k2H, k2I, \
                k30, k31, k32, k33, k34, k35, k36, k37, k38, k39, k3A, k3B, k3C, k3D, k3E, k3F, k3G, k3H, k3I, \
                k40, k41, k42, k43, k44, k45, k46, k47, k48, k49, k4A, k4B, k4C, k4D, k4E, k4F, k4G, k4H, k4I  \
              ) \
{ \
  { k09  , k08  , k07  , k05  , k04  , k03  , k02  , k01  , k00   }, /* elec row 0 (KiCad ROW 1) */ \
  { k19  , k18  , k17  , k15  , k14  , k13  , k12  , k11  , k10   }, /* elec row 1 (KiCad ROW 2) */ \
  { k28  , k27  , k26  , k25  , k24  , k23  , k22  , k21  , k20   }, /* elec row 2 (KiCad ROW 3) */ \
  { k37  , k46  , k36  , k35  , k34  , k33  , k32  , k31  , k30   }, /* elec row 3 (KiCad ROW 4) */ \
  { k49  , k48  , k47  , k45  , k44  , k43  , k42  , k41  , k40   }, /* elec row 4 (KiCad ROW 5) */ \
  { k0A  , k0B  , k0C  , k1D  , k1E  , k0F  , k0G  , k0H  , k0I  }, /* elec row 5 (KiCad ROW 6) */ \
  { k1A  , k1B  , k1C  , k2D  , k2E  , k1F  , k1G  , k1H  , k1I  }, /* elec row 6 (KiCad ROW 7) */ \
  { k29  , k2A  , k2B  , k2C  , k3D  , k2F  , k2G  , k2H  , k2I  }, /* elec row 7 (KiCad ROW 8) */ \
  { k38  , k39  , k3A  , k3B  , k3C  , k3F  , k3G  , k3H  , k3I  }, /* elec row 8 (KiCad ROW 9) */ \
  { k4A  , k4B  , k4C  , k4D  , k3E  , k4E  , k4G  , k4H  , k4I  }  /* elec row 9 (KiCad ROW10) */ \
}

#define NUM_LAYERS  3

/* ============================================================================
   Keymap — 3 layers, stored in flash
   ========================================================================= */
const uint16_t PROGMEM keymaps[NUM_LAYERS][MATRIX_ROWS][MATRIX_COLS] =
{

  /* ------------------------------------------------------------------
     Layer 0 — Base

     Physical layout reference (phys cols 0–18, rows 0–4):

       c0    c1    c2    c3    c4    c5    c6    c7    c8    c9   c10   c11   c12   c13   c14   c15   c16   c17   c18
       ────────────────────────────────────────────────────────────────────────────────────────────────────────────────
       ESC   1     2     3     4     5    [ ]    6     7     8     9     0     -     =    Bspc  Ins   Home  PgUp  NLk
       Tab   Q     W     E     R     T    [ ]    Y     U     I     O     P     [     ]     \    Del   End   PgDn   /
       Caps  A     S     D     F     G     H     J     K     L     ;     '    Ent   [ ]   [ ]    *     -     +    P4
       LSft  Z     X     C     V     B     N     M     ,     .     /    RSft  [ ]   [ ]   Up    [ ]   P1    P2    P3
       LCtl LGui LAlt  Spc   [ ]   [ ]   [ ]  RAlt  Fn   RCtl  [ ]   [ ]   [ ]  Left  Down  Right  P0   P.   PEnt

     Numpad detail (cols 15–18):
       col15: NLk   /    *    -    [ ]    ← 4-key strip, row4 empty
       col16:  7    4    1    +    P0     ← P+ has one switch (2u key)
       col17:  8    5    2    P.  PEnt
       col18:  9    6    3  PEnt PEnt    ← two switches for 2u Enter key

     Invariant: modifier positions on layers 1 and 2 must be KC_TRNS.
     ------------------------------------------------------------------ */
  [0] = KEYMAP(
    /* row 0 */ KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    XXXXXXX, KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, XXXXXXX, XXXXXXX, KC_INS,  KC_HOME, KC_PGUP, KC_NLCK,
    /* row 1 */ KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    XXXXXXX, KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS, KC_DEL,  KC_END,  KC_PGDN, KC_PSLS,
    /* row 2 */ KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,  XXXXXXX, XXXXXXX, KC_PAST, KC_PMNS, KC_PPLS, KC_P4,
    /* row 3 */ KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, XXXXXXX, XXXXXXX, KC_UP,   XXXXXXX, KC_P1,   KC_P2,   KC_P3,
    /* row 4 */ KC_LCTL, KC_LGUI, KC_LALT, KC_SPC,  XXXXXXX, XXXXXXX, XXXXXXX, KC_RALT, LY_MO1,  KC_RCTL, XXXXXXX, XXXXXXX, XXXXXXX, KC_LEFT, KC_DOWN, XXXXXXX, KC_P0,   KC_PDOT, KC_PENT
  ),

  /* ------------------------------------------------------------------
     Layer 1 — Function

     Invariant: all modifier positions must be KC_TRNS so that
     modifier+Fn combos work regardless of press order.
     ------------------------------------------------------------------ */
  [1] = KEYMAP(
    /* row 0 */ ______,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   XXXXXXX, KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  XXXXXXX, XXXXXXX, ______,  ______,  ______,  ______,
    /* row 1 */ ______,  MC_COPY, MC_PSTE, MC_CUT,  CC_VOLU, CC_VOLD, XXXXXXX, CC_MUTE, ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,
    /* row 2 */ ______,  MC_SALL, MC_SAVE, MC_FIND, ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, ______,  ______,  ______,  ______,
    /* row 3 */ ______,  MC_UNDO, MC_REDO, ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, CC_MPLY, XXXXXXX, ______,  ______,  ______,
    /* row 4 */ ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, XXXXXXX, ______,  ______,  LY_MO2,  XXXXXXX, XXXXXXX, XXXXXXX, CC_MPRV, CC_MSTP, XXXXXXX, ______,  ______,  ______
  ),

  /* ------------------------------------------------------------------
     Layer 2 — System / config
     ------------------------------------------------------------------ */
  [2] = KEYMAP(
    /* row 0 */ SYS_BOOT,SYS_RST, ______,  ______,  ______,  ______,  XXXXXXX, ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, ______,  ______,  ______,  ______,
    /* row 1 */ ______,  ______,  ______,  ______,  LD_BRIU, LD_BRID, XXXXXXX, ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,
    /* row 2 */ ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, ______,  ______,  ______,  ______,
    /* row 3 */ ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, ______,  XXXXXXX, ______,  ______,  ______,
    /* row 4 */ ______,  ______,  ______,  ______,  XXXXXXX, XXXXXXX, XXXXXXX, ______,  ______,  ______,  XXXXXXX, XXXXXXX, XXXXXXX, ______,  ______,  XXXXXXX, KC_F10,  KC_F11,  KC_F12
  ),
};

/* ============================================================================
   keymap_key_to_keycode — layer-aware lookup with transparent fallthrough
   ========================================================================= */
uint16_t keymap_key_to_keycode(uint8_t layer, uint8_t row, uint8_t col)
{ if(layer >= NUM_LAYERS || row >= MATRIX_ROWS || col >= MATRIX_COLS)
    return KC_NO;

  uint16_t kc = pgm_read_word(&keymaps[layer][row][col]);

  while(kc == KC_TRNS && layer > 0)
  { layer--;
    kc = pgm_read_word(&keymaps[layer][row][col]);
  }

  return (kc == KC_TRNS) ? KC_NO : kc;
}

/* ============================================================================
   Alt-Tab encoder behaviour

   encoder_step() is called by encoder.c on each confirmed detent.
   keymap_tick()  is called once per 1 kHz tick by keyboard_task().

   The shared keyboard report is accessed exclusively through kbd_*() so
   that all writes go through the seqlock in hid_kbd_stage().  This is the
   fix for bug #13.
   ========================================================================= */

#define ALT_RELEASE_TIMEOUT_MS  600u

static uint8_t  s_alt_held;
static uint16_t s_alt_idle_ticks;

/* --------------------------------------------------------------------------
   encoder_step — CW: Alt+Tab forward.  CCW: Alt+Shift+Tab backward.

   Alt is held across steps and released by keymap_tick() after
   ALT_RELEASE_TIMEOUT_MS of encoder inactivity.
   ------------------------------------------------------------------------ */
void encoder_step(int8_t dir)
{ /* Latch Alt on the first step of a new gesture. */
  if(!s_alt_held)
  { kbd_set_mod(MOD_LALT);
    kbd_stage();
    s_alt_held = 1;
  }

  s_alt_idle_ticks = 0;

  if(dir == ENCODER_CW)
  { /* Forward: tap Tab while Alt is held. */
    kbd_add_key(KC_TAB);
    kbd_stage();
    kbd_remove_key(KC_TAB);
    kbd_stage();

  }
  else
  { /* Backward: tap Shift+Tab while Alt is held.
       Assert Shift, tap Tab, clear Shift — Alt stays latched. */
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
   encoder_alt_timeout_tick — call once per 1 kHz tick, after encoder_scan().
   Releases Alt when encoder goes idle for ALT_RELEASE_TIMEOUT_MS.
   ------------------------------------------------------------------------ */
void encoder_alt_timeout_tick(void)
{ if(!s_alt_held)
    return;

  s_alt_idle_ticks++;

  if(s_alt_idle_ticks >= ALT_RELEASE_TIMEOUT_MS)
  { kbd_clear_mod(MOD_LALT);
    kbd_stage();
    s_alt_held       = 0;
    s_alt_idle_ticks = 0;
  }
}
