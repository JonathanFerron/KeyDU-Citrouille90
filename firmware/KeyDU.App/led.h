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
#define LED_BRIGHTNESS_DEFAULT  128      // 50% on power-up
#define LED_BRIGHTNESS_STEP     16       // ~6% per encoder click
#define LED_BRIGHTNESS_DELTA    48       // Per-layer brightness offset

extern uint8_t led_brightness;
extern bool    led_enabled;

void led_init(void);
void led_set(uint8_t brightness);
void led_step(bool dir, uint8_t step);
void led_update_layer(uint8_t layer);
void led_off(void);
void led_on(void);

#endif // LED_H
