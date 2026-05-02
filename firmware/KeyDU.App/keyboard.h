/* keyboard.h — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Public interface for the top-level keyboard logic module.
 *
 * Report manipulation API
 * -----------------------
 * keyboard.c owns the single hid_kbd_report_t and hid_consumer_report_t
 * that represent the current keyboard and consumer state.  All modules
 * (keymap.c, macro.c) that need to modify reports call through this API
 * rather than touching report structs directly.  This ensures all writes
 * go through the seqlock in hid_kbd_stage() / hid_consumer_stage().
 *
 * Keyboard report:
 *   kbd_set_mod()     — OR mod_bits into the modifier byte
 *   kbd_clear_mod()   — AND ~mod_bits out of the modifier byte
 *   kbd_add_key()     — insert keycode into the first free slot (6KRO)
 *   kbd_remove_key()  — clear keycode from its slot
 *   kbd_stage()       — push current report to the USB double-buffer
 *
 * Consumer report:
 *   kbd_consumer_set()   — set consumer usage code and stage
 *   kbd_consumer_clear() — clear consumer usage and stage
 *
 * Lifecycle:
 *   keyboard_init() — initialise all subsystems; call once from main()
 *   keyboard_task() — one 1 kHz scan tick; call from main() when tick flag set
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* ── Subsystem lifecycle ──────────────────────────────────────────────── */

/* Initialise matrix, layer, led, encoder, USB.
 * Call once from main() before entering the main loop.
 * Assumes wdt_disable() has already been called. */
void keyboard_init(void);

/* Process one 1 kHz scan tick.
 * Call exactly once per tick from main(). */
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
 * Non-blocking.  Safe to call from scan-loop context. */
void kbd_stage(void);

/* ── Consumer report API ──────────────────────────────────────────────── */

/* Set the consumer usage field (HID Consumer page, 16-bit) and stage.
 * Pass the CC_* keycode — the mapping to HID usage is done internally. */
void kbd_consumer_set(uint16_t cc_keycode);

/* Clear the consumer usage field and stage a zero report. */
void kbd_consumer_clear(void);

#endif /* KEYBOARD_H */
