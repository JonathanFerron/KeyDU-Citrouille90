/* keyboard.h — Citrouille90 / AVR64DU32
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   Public interface for the top-level keyboard logic module.

   Report manipulation API
   -----------------------
   keyboard.c owns the single hid_kbd_report_t and hid_consumer_report_t
   that represent the current keyboard and consumer state.  All modules
   (keymap.c, macro.c, compose.c) that need to modify reports call through
   this API rather than touching report structs directly.

   Keyboard report:
     kbd_set_mod()     — OR mod_bits into the modifier byte
     kbd_clear_mod()   — AND ~mod_bits out of the modifier byte
     kbd_add_key()     — insert keycode into the first free slot (6KRO)
     kbd_remove_key()  — clear keycode from its slot
     kbd_stage()       — push current report to the USB double-buffer
                         Phase 1: writes to seqlock double-buffer (may race
                         if called multiple times per SOF interval).
                         Phase 2: enqueues into kbd report queue — non-blocking,
                         hid_flush() drains one entry per SOF.  See usb_hid.h.

   Shared key-send helpers (used by macro.c and compose.c):
     send_mod_key()    — press mod+key, stage, release, stage
     tap_key()         — press key, stage, release, stage

   Both helpers call kbd_stage() twice.  Under phase 1 this may race with
   the SOF ISR if multiple calls occur within one tick.  Under phase 2 the
   queue makes each staged report visible to hid_flush() independently,
   eliminating the race.

   Consumer report:
     kbd_consumer_set()   — set consumer usage code and stage
     kbd_consumer_clear() — clear consumer usage and stage

   Lifecycle:
     keyboard_init() — initialise all subsystems; call once from main()
     keyboard_task() — one 1 kHz scan tick; call from main() when tick flag set
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#include "usb_hid.h"

/* ── Subsystem lifecycle ──────────────────────────────────────────────── */

/* Initialise matrix, layer, led, encoder, USB.
   Call once from main() before entering the main loop.
   Assumes wdt_disable() has already been called. */
void keyboard_init(void);

/* Process one 1 kHz scan tick.
   Call exactly once per tick from main(). */
void keyboard_task(void);

/* ── Keyboard report API ──────────────────────────────────────────────── */

/* Set modifier bits (OR into current modifier byte). */
void kbd_set_mod(uint8_t mod_bits);

/* Clear modifier bits (AND ~mod_bits out of modifier byte). */
void kbd_clear_mod(uint8_t mod_bits);

/* Add a basic keycode to the first free slot.  No-op if all 6 slots full. */
void kbd_add_key(uint8_t keycode);

/* Remove a basic keycode from its slot.  No-op if not present. */
void kbd_remove_key(uint8_t keycode);

/* Push the current keyboard report to the USB seqlock buffer.
   Non-blocking.  Safe to call from scan-loop context. */
void kbd_stage(void);

/* Send a modifier + key combo: press, stage, release, stage.
   mod_bits: MOD_* bitmask (0 for no modifier — plain key tap).
   keycode:  basic HID keycode (KC_*).
   Used by macro.c and compose.c — lifted here so both share one
   implementation and the phase 2 queue fix applies to all callers. */
void send_mod_key(uint8_t mod_bits, uint8_t keycode);

/* Tap a single key: press, stage, release, stage.
   Equivalent to send_mod_key(0, kc) but named for clarity at call sites. */
void tap_key(uint8_t kc);

/* ── Consumer report API ──────────────────────────────────────────────── */

/* Set the consumer usage field (HID Consumer page, 16-bit) and stage.
   Pass the CC_* keycode — the mapping to HID usage is done internally. */
void kbd_consumer_set(uint16_t cc_keycode);

/* Clear the consumer usage field and stage a zero report. */
void kbd_consumer_clear(void);

/* Return a snapshot of the current keyboard report (for GET_REPORT). */
void kbd_get_report(hid_kbd_report_t* out);

#endif /* KEYBOARD_H */
