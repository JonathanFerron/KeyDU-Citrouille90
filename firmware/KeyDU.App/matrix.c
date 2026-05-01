/* matrix.c — Key matrix scan for Citrouille90 / AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Scanning scheme: column-output-low / row-input-pullup, active-low.
 *
 * Pin assignment (32-pin package):
 *
 *   Columns (output, driven low one at a time)
 *     PF0–PF5  →  cols 0–5   (COL_F_MASK 0x3F)
 *     PD5–PD7  →  cols 6–8   (COL_D_MASK 0xE0)
 *
 *   Rows (input, pull-up enabled, active-low)
 *     PD0–PD4  →  rows 0–4   (ROW_D_MASK 0x1F)
 *     PA0      →  row  9
 *     PA1      →  row  8
 *     PA2      →  row  7
 *     PA3      →  row  5
 *     PA6      →  row  6     (ROW_A_MASK 0x4F)
 *
 * Note: PORTD is mixed — PD0–PD4 are row inputs, PD5–PD7 are col outputs.
 *
 * Debounce
 * --------
 * Symmetric counter debounce: a raw reading must differ from debounced_state
 * for DEBOUNCE_TICKS consecutive ticks before the debounced state commits.
 * Any tick where raw == debounced resets the counter.
 *
 * DEBOUNCE_TICKS is defined in matrix.h.
 * At 1 kHz scan rate, 1 tick = 1 ms.  4 ticks → 3 ms debounce window,
 * within the Kailh Choc V1 spec (<5 ms bounce).
 *
 * State arrays
 * ------------
 * debounced_state[row][col]  — committed stable state (internal only)
 * matrix_current[row][col]   — debounced state snapshotted this tick
 * matrix_previous[row][col]  — debounced state snapshotted last tick
 *
 * All three are static.  External callers use matrix_is_pressed(),
 * matrix_is_key_pressed(), matrix_is_key_released().
 */

#include <avr/io.h>
#include <stdbool.h>
#include <string.h>

#include "gpio.h"
#include "matrix.h"

// ============================================================================
// Matrix Pin Definitions (AVR64DU32 specific): are these detailed defines still required?
// ============================================================================
// Row pins (outputs)
#define ROW0_PIN            PORTA_PIN0
#define ROW1_PIN            PORTA_PIN1
#define ROW2_PIN            PORTA_PIN2
#define ROW3_PIN            PORTA_PIN3
#define ROW4_PIN            PORTA_PIN4
#define ROW5_PIN            PORTA_PIN5
#define ROW6_PIN            PORTA_PIN6
#define ROW7_PIN            PORTA_PIN7
#define ROW8_PIN            PORTB_PIN0
#define ROW9_PIN            PORTB_PIN1

// Column pins (inputs with pullups)
#define COL0_PIN            PORTB_PIN2
#define COL1_PIN            PORTB_PIN3
#define COL2_PIN            PORTB_PIN4
#define COL3_PIN            PORTB_PIN5
#define COL4_PIN            PORTF_PIN0
#define COL5_PIN            PORTF_PIN1
#define COL6_PIN            PORTF_PIN2
#define COL7_PIN            PORTF_PIN3
#define COL8_PIN            PORTF_PIN4

/* ============================================================================
 * Port-wide masks
 * ========================================================================= */
#define COL_F_MASK  0x3F    /* PF0–PF5  cols 0–5  */
#define COL_D_MASK  0xE0    /* PD5–PD7  cols 6–8  */
#define ROW_D_MASK  0x1F    /* PD0–PD4  rows 0–4  */
#define ROW_A_MASK  0x4F    /* PA0–PA3, PA6: rows 5–9 mapped non-contiguously to those 5 pins on Port A */

/* ============================================================================
 * Column pin descriptors (used in init and scan)
 * ========================================================================= */
static const gpio_pin_t col_pins[MATRIX_COLS] = {
    GPIO_PIN(PORTF, 0),   /* col 0 */
    GPIO_PIN(PORTF, 1),   /* col 1 */
    GPIO_PIN(PORTF, 2),   /* col 2 */
    GPIO_PIN(PORTF, 3),   /* col 3 */
    GPIO_PIN(PORTF, 4),   /* col 4 */
    GPIO_PIN(PORTF, 5),   /* col 5 */
    GPIO_PIN(PORTD, 5),   /* col 6 */
    GPIO_PIN(PORTD, 6),   /* col 7 */
    GPIO_PIN(PORTD, 7),   /* col 8 */
};

/* ============================================================================
 * PORTA row bit → logical row index
 * Index = pin number (0–7); value = row index (0-based); 0xFF = unused.
 *   PA0 → row 9   PA1 → row 8   PA2 → row 7
 *   PA3 → row 5   PA6 → row 6
 * ========================================================================= */
static const uint8_t row_a_lut[8] = {
    9, 8, 7, 5, 0xFF, 0xFF, 6, 0xFF
};

/* ============================================================================
 * Matrix state — all static, not exposed in the header
 * ========================================================================= */
static bool debounced_state[MATRIX_ROWS][MATRIX_COLS];  /* committed stable  */
static bool matrix_current[MATRIX_ROWS][MATRIX_COLS];   /* this tick         */
static bool matrix_previous[MATRIX_ROWS][MATRIX_COLS];  /* previous tick     */
static uint8_t debounce_counters[MATRIX_ROWS][MATRIX_COLS];

/* ============================================================================
 * matrix_init
 * ========================================================================= */
void matrix_init(void)
{
    /* Columns: latch high before enabling output to avoid a glitch. */
    PORTF.OUTSET = COL_F_MASK;
    PORTF.DIRSET = COL_F_MASK;
    PORTD.OUTSET = COL_D_MASK;
    PORTD.DIRSET = COL_D_MASK;

    /* Disable input buffers on column output pins (power / noise reduction). */
    GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTF, COL_F_MASK);
    GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(PORTD, COL_D_MASK);

    /* Rows: inputs at reset by default — enable pull-ups only.
     * PINCONFIG is mirrored across all ports; one write stages it for both. */
    GPIO_MULTIPIN_PULLUP(PORTA, ROW_A_MASK);
    GPIO_MULTIPIN_PULLUP(PORTD, ROW_D_MASK);

    /* Zero all state arrays. */
    memset(debounced_state,    0, sizeof(debounced_state));
    memset(matrix_current,     0, sizeof(matrix_current));
    memset(matrix_previous,    0, sizeof(matrix_previous));
    memset(debounce_counters,  0, sizeof(debounce_counters));
}

/* ============================================================================
 * scan_col_raw — drive one column low, read both row ports, release column.
 *
 * Returns raw (un-debounced) row state for the column as a bitmask:
 *   bits 0–4  correspond to rows 0–4  (PORTD PD0–PD4)
 *   bits 5–9  correspond to rows 5–9  (PORTA, resolved via row_a_lut)
 *
 * Bit set (1) means the switch is pressed (pin read low, active-low).
 * ========================================================================= */
static uint16_t scan_col_raw(const gpio_pin_t col)
{
    GPIO_LOW(col);
    __builtin_avr_nop();                         /* RC settle */

    uint8_t  rows_d = ~VPORTD.IN & ROW_D_MASK;  /* single-cycle IN read */
    uint8_t  rows_a = ~VPORTA.IN & ROW_A_MASK;  /* single-cycle IN read */

    GPIO_HIGH(col);

    uint16_t raw = 0;

    /* PORTD rows: PD0–PD4 map directly to row indices 0–4. */
    raw |= (uint16_t)rows_d;   /* bits 0–4 already in the right positions */

    /* PORTA rows: non-contiguous pins resolved via LUT, placed in bits 5–9. */
    uint8_t ra = rows_a;
    while (ra) {
        uint8_t bit   = ra & -ra;                /* isolate lowest set bit */
        uint8_t row   = row_a_lut[__builtin_ctz(bit)];
        if (row != 0xFF) {
            raw |= (uint16_t)(1u << row);
        }
        ra &= ra - 1;                            /* clear lowest set bit   */
    }

    return raw;
}

/* ============================================================================
 * matrix_scan — call once per 1 kHz tick
 *
 * For each column:
 *   1. Read raw pressed state for all rows in that column.
 *   2. Apply symmetric counter debounce per cell.
 *   3. Update matrix_current from debounced_state.
 * Then snapshot matrix_previous from last tick's matrix_current.
 * ========================================================================= */
void matrix_scan(void)
{
    /* Advance the state frame before updating current. */
    memcpy(matrix_previous, matrix_current, sizeof(matrix_previous));

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        uint16_t raw = scan_col_raw(col_pins[col]);

        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            bool raw_pressed = (raw >> row) & 1u;

            if (raw_pressed != debounced_state[row][col]) {
                /* Raw disagrees with committed state — increment counter.
                 * Commit when the disagreement holds for DEBOUNCE_TICKS. */
                debounce_counters[row][col]++;
                if (debounce_counters[row][col] >= DEBOUNCE_TICKS) {
                    debounced_state[row][col]   = raw_pressed;
                    debounce_counters[row][col] = 0;
                }
            } else {
                /* Raw agrees with committed state — reset counter. */
                debounce_counters[row][col] = 0;
            }

            matrix_current[row][col] = debounced_state[row][col];
        }
    }
}

/* ============================================================================
 * Query functions
 * ========================================================================= */

/* True if key is currently debounced-pressed. */
bool matrix_is_pressed(uint8_t row, uint8_t col)
{
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return false;
    }
    return matrix_current[row][col];
}

/* True for exactly one tick on a press edge (low→high). */
bool matrix_is_key_pressed(uint8_t row, uint8_t col)
{
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return false;
    }
    return matrix_current[row][col] && !matrix_previous[row][col];
}

/* True for exactly one tick on a release edge (high→low). */
bool matrix_is_key_released(uint8_t row, uint8_t col)
{
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return false;
    }
    return !matrix_current[row][col] && matrix_previous[row][col];
}
