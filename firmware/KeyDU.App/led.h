// ============================================================================
// led.h
// ============================================================================

#ifndef LED_H
#define LED_H

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// LED Configuration
// ============================================================================

#define LED_PORT                PORTA
#define LED_A_PIN               4        // PA4 = TCA0 WO4 (HCMP1)
#define LED_B_PIN               5        // PA5 = TCA0 WO5 (HCMP2)

#define LED_BRIGHTNESS_MIN      0
#define LED_BRIGHTNESS_MAX      255

/* Perceptual brightness table — 30-entry geometric sequence (ratio 1.133, k=0.133 Weber).
   Each step is one JND (~16 PWM at mid-range).  Index 0 = off, index 29 = full. */
#define LED_TABLE_SIZE          25
#define LED_INDEX_DEFAULT       12   // PWM 42 at power-on
#define LED_DELTA_STEPS          2   // layer feedback: 2 JND steps up from user base

extern bool led_enabled;

void led_init(void);
void led_set(uint8_t index);
void led_step(bool dir);
void led_update_layer(uint8_t layer);
void led_apply_host_report(uint8_t led_report);  /* bit0=NumLock→LED A full, bit1=CapsLock→LED B full */
void led_off(void);
void led_on(void);

#endif // LED_H
