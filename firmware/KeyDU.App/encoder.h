/* encoder.h — Rotary encoder interface for Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/* Initialise encoder pins as plain inputs (no pull-up — external RC filter
 * and 10kΩ pull-up to VDD per Bourns application note; see scanning
 * decisions doc section 3). */
void encoder_init(void);

/* Scan encoder quadrature state. Call once per 1KHz tick from the main
 * scan loop, after matrix_scan().
 * Emits encoder_step(dir) when a full quadrature cycle is confirmed. */
void encoder_scan(void);

/* Direction constants passed to encoder_step(). */
#define ENCODER_CW   ( 1)
#define ENCODER_CCW  (-1)

/* Provided by the keymap/event layer — called when a full detent step is
 * confirmed.  dir is ENCODER_CW or ENCODER_CCW. */
void encoder_step(int8_t dir);

#endif /* ENCODER_H */
