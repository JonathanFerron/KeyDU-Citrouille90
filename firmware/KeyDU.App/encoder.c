/* encoder.c — Rotary encoder scan for Citrouille90 / AVR64DU32
 * Bourns PES12 quadrature encoder, 16-entry LUT state machine.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Pin assignment (32-pin package):
 *
 *   ENC_A  →  PA7   (encoder terminal A)
 *   ENC_B  →  PC3   (encoder terminal B)
 *
 * Both pins are plain inputs — no internal pull-up.  External RC filter
 * (10kΩ pull-up to VDD, 10kΩ series, 10nF to GND) handles bounce
 * suppression electrically; see scanning decisions doc sections 3–4.
 *
 * Algorithm: 16-entry LUT quadrature decode, ±4 accumulator threshold,
 * accumulator reset on illegal/no-change, post-step lockout timer.
 * See scanning decisions doc section 6 for full rationale.
 */

#include <avr/io.h>
#include <stdint.h>
#include "gpio.h"
#include "encoder.h"

/* ---------------------------------------------------------
 * Encoder pin descriptors
 * --------------------------------------------------------- */
#define ENC_A_PORT    PORTA
#define ENC_A_VPORT   VPORTA
#define ENC_A_PIN     7
#define ENC_B_PORT    PORTC
#define ENC_B_VPORT   VPORTC
#define ENC_B_PIN     3

static const gpio_pin_t ENC_A = GPIO_PIN(ENC_A_PORT, ENC_A_PIN);   /* PA7 */
static const gpio_pin_t ENC_B = GPIO_PIN(ENC_B_PORT, ENC_B_PIN);   /* PC3 */

/* ---------------------------------------------------------
 * Quadrature lookup table
 * index = (prev_ab << 2) | curr_ab
 * +1 = CW, -1 = CCW, 0 = no change or illegal transition
 * --------------------------------------------------------- */
static const int8_t enc_lut[16] = {
     0,  /* 0000: 00→00  no change     */
    -1,  /* 0001: 00→01  valid CCW     */
    +1,  /* 0010: 00→10  valid CW      */
     0,  /* 0011: 00→11  illegal       */
    +1,  /* 0100: 01→00  valid CW      */
     0,  /* 0101: 01→01  no change     */
     0,  /* 0110: 01→10  illegal       */
    -1,  /* 0111: 01→11  valid CCW     */
    -1,  /* 1000: 10→00  valid CCW     */
     0,  /* 1001: 10→01  illegal       */
     0,  /* 1010: 10→10  no change     */
    +1,  /* 1011: 10→11  valid CW      */
     0,  /* 1100: 11→00  illegal       */
    +1,  /* 1101: 11→01  valid CW     */
    -1,  /* 1110: 11→10  valid CCW      */
     0,  /* 1111: 11→11  no change     */
};

/* Post-step lockout duration in scan ticks (1 tick = 1ms at 1KHz). */
#define LOCKOUT_MS  2

/* ---------------------------------------------------------
 * Persistent decoder state
 * --------------------------------------------------------- */
static uint8_t  s_prev_ab;        /* last confirmed good AB reading     */
static int8_t   s_accumulator;    /* signed quadrature accumulator      */
static uint8_t  s_lockout_timer;  /* ticks remaining in post-step lockout */

/* ---------------------------------------------------------
 * encoder_init
 * --------------------------------------------------------- */
void encoder_init(void) {
    /* Plain inputs — DIRCLR is reset default, explicit for clarity */
    GPIO_SET_INPUT(ENC_A);
    GPIO_SET_INPUT(ENC_B);

    /* No internal pull-up: external RC filter provides pull-up to VDD.
     * Enabling the internal pull-up would shift the RC time constant
     * (parallel resistance with the 10kΩ external), see decisions doc §3. */
    GPIO_PULLUP_DISABLE(ENC_A);
    GPIO_PULLUP_DISABLE(ENC_B);

    /* Seed state to current pin values to avoid a phantom step on first scan */
    uint8_t a = GPIO_VPORT_READ(ENC_A_VPORT, ENC_A_PIN);
    uint8_t b = GPIO_VPORT_READ(ENC_B_VPORT, ENC_B_PIN);
    s_prev_ab       = (a << 1) | b;
    s_accumulator   = 0;
    s_lockout_timer = 0;
}

/* ---------------------------------------------------------
 * encoder_scan — call once per 1KHz tick
 *
 * Faithful implementation of the pseudocode in scanning decisions §6.
 * Ordering is intentional — see Key Design Notes in that document.
 * --------------------------------------------------------- */
void encoder_scan(void) {

    /* Step 1: decrement lockout timer unconditionally */
    if (s_lockout_timer > 0) {
        s_lockout_timer--;
    }

    /* Step 2: early exit while still locked out
     * prev_ab is held at the last confirmed good state — the next valid
     * reading after lockout expires will be evaluated against a clean
     * baseline rather than a potentially glitchy mid-lockout snapshot. */
    if (s_lockout_timer > 0) {
        return;
    }

    /* Step 3: read current pin state (deferred until after lockout check) */    
    uint8_t curr_ab = (GPIO_VPORT_READ(ENC_A_VPORT, ENC_A_PIN) << 1)   /* PA7 → bit 1 */
                     | GPIO_VPORT_READ(ENC_B_VPORT, ENC_B_PIN);        /* PC3 → bit 0 */

    /* Step 4: no change — early exit, nothing to decode */
    if (curr_ab == s_prev_ab) {
        return;
    }

    /* Step 5: look up transition delta */
    uint8_t index = (uint8_t)((s_prev_ab << 2) | curr_ab);
    int8_t  delta = enc_lut[index];

    /* Step 6: illegal transition (or no-change entry — unreachable here but
     * handled for table completeness) — reset accumulator and stay anchored
     * to the last confirmed good state (prev_ab NOT updated). */
    if (delta == 0) {
        s_accumulator = 0;
        return;
    }

    /* Step 7: valid transition — accumulate and advance state */
    s_accumulator += delta;
    s_prev_ab      = curr_ab;

    /* Step 8: check for completed full quadrature cycle (±4 threshold) */
    if (s_accumulator >= +4) {
        s_accumulator   = 0;
        s_lockout_timer = LOCKOUT_MS;
        encoder_step(ENCODER_CW);

    } else if (s_accumulator <= -4) {
        s_accumulator   = 0;
        s_lockout_timer = LOCKOUT_MS;
        encoder_step(ENCODER_CCW);
    }
}
