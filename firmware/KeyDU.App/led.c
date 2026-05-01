// ============================================================================
// led.c
// ============================================================================

#include "led.h"

uint8_t led_brightness = LED_BRIGHTNESS_DEFAULT;
static uint8_t led_level_a    = LED_BRIGHTNESS_DEFAULT;
static uint8_t led_level_b    = LED_BRIGHTNESS_DEFAULT;
bool    led_enabled    = true;

static void led_write(uint8_t a, uint8_t b)
{
  TCA0.SPLIT.HCMP1 = a;  // PA4 / WO4 — takes effect at next HCNT underflow
  TCA0.SPLIT.HCMP2 = b;  // PA5 / WO5
}

void led_init(void)
{
  LED_PORT.DIRSET = (1 << LED_A_PIN) | (1 << LED_B_PIN);

  // PORTA is the hardware default for TCA0 — stated explicitly for clarity
  PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;

  // Enable split mode first — must precede all other TCA0 register writes
  TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

  TCA0.SPLIT.HPER = LED_BRIGHTNESS_MAX;  // 8-bit period for high counter

  led_write(led_brightness, led_brightness);

  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm;

  // f_PWM = 24 MHz / (64 × 256) ≈ 1.465 kHz
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm;
}

void led_set(uint8_t brightness)
{
  led_brightness = brightness;
  led_level_a    = brightness;
  led_level_b    = brightness;
  if (led_enabled) led_write(led_level_a, led_level_b);
}

void led_step(bool dir, uint8_t step)
{
  if (dir) {
    led_brightness = (led_brightness > LED_BRIGHTNESS_MAX - step)
      ? LED_BRIGHTNESS_MAX
      : led_brightness + step;
  } else {
    led_brightness = (led_brightness < LED_BRIGHTNESS_MIN + step)
      ? LED_BRIGHTNESS_MIN
      : led_brightness - step;
  }

  led_level_a = led_brightness;
  led_level_b = led_brightness;
  if (led_enabled) led_write(led_level_a, led_level_b);
}

void led_update_layer(uint8_t layer)
{
  uint16_t v;

  switch (layer) {
    case 0:
      led_level_a = led_brightness;
      led_level_b = led_brightness;
      break;
    case 1:
      v = (uint16_t)led_brightness + LED_BRIGHTNESS_DELTA;
      led_level_a = (v > LED_BRIGHTNESS_MAX) ? LED_BRIGHTNESS_MAX : (uint8_t)v;
      led_level_b = led_brightness;
      break;
    default:
      v = (uint16_t)led_brightness + LED_BRIGHTNESS_DELTA;
      led_level_a = (v > LED_BRIGHTNESS_MAX) ? LED_BRIGHTNESS_MAX : (uint8_t)v;
      led_level_b = led_level_a;
      break;
  }
  if (led_enabled) led_write(led_level_a, led_level_b);
}

void led_off(void)
{
  led_enabled = false;
  TCA0.SPLIT.CTRLB &= ~(TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm);
  LED_PORT.OUTCLR = (1 << LED_A_PIN) | (1 << LED_B_PIN);
}

void led_on(void)
{
  led_enabled = true;
  led_write(led_level_a, led_level_b);
  TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm;
}
