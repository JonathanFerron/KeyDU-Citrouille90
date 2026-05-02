 /* keymap.h — Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Public interface for keymap.c.
 * Owns layer-aware keycode lookup and the encoder Alt-Tab tick.
 */

#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>

/* Return the resolved keycode for (layer, row, col) with transparent
 * fallthrough to lower layers.  Returns KC_NO if transparent on all layers
 * or any index is out of range. */
uint16_t keymap_key_to_keycode(uint8_t layer, uint8_t row, uint8_t col);

/* Call once per 1 kHz tick from keyboard_task(), after encoder_scan().
 * Manages the Alt-release timeout for the encoder Alt-Tab gesture. */
void keymap_tick(void);

#endif /* KEYMAP_H */
