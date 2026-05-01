/* keyboard.h — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Public interface for the top-level keyboard logic module.
 * keyboard.c owns subsystem init, the 1 kHz task loop, and key
 * event dispatch.  It also provides register_key(), the callback
 * required by matrix.c.
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* Initialise all subsystems (matrix, layer, led, encoder).
 * Call once from main() before entering the main loop.
 * Assumes wdt_disable() and sei() have already been called by main(). */
void keyboard_init(void);

/* Process one scan tick.  Call exactly once per 1 kHz tick from main().
 * Drives matrix_scan(), encoder_scan(), keymap_tick(), key event dispatch,
 * LED update, and usb_task(). */
void keyboard_task(void);

#endif /* KEYBOARD_H */
