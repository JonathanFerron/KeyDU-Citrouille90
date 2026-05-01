/* matrix.h — Key matrix interface for Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * matrix_scan() performs a full column-output-low / row-input-pullup scan,
 * applies per-key debounce, and maintains debounced current and previous
 * state arrays.  After matrix_scan() returns, callers may query any key's
 * edge or level state via the functions below.
 *
 * There is no register_key() callback in this model.  Edge detection lives
 * entirely inside matrix.c; keyboard.c consumes it through matrix_is_key_*().
 */

#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stdbool.h>

/* Matrix dimensions — electrical layout. */
#define MATRIX_COLS     9
#define MATRIX_ROWS     10
#define MATRIX_KEYS         (MATRIX_ROWS * MATRIX_COLS)  // 90 keys

// Debounce time in milliseconds
#define DEBOUNCE_TICKS         4

/* Initialise column outputs and row input pull-ups, zero all state. */
void matrix_init(void);

/* Scan the full matrix once.  Call exactly once per 1 kHz tick.
 * Updates internal debounced current/previous state arrays.
 * After this returns, use the query functions below. */
void matrix_scan(void);

/* Level query: true if key is currently debounced-pressed. */
bool matrix_is_pressed(uint8_t row, uint8_t col);

/* Edge queries: true for exactly one tick on a transition. */
bool matrix_is_key_pressed(uint8_t row, uint8_t col);   /* low → high edge */
bool matrix_is_key_released(uint8_t row, uint8_t col);  /* high → low edge */

#endif /* MATRIX_H */
