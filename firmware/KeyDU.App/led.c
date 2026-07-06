// ============================================================================
// led.c
// ============================================================================

#include "led.h"
#include <avr/pgmspace.h>

/* Perceptual brightness table: I_n = round(8 * 1.162^n), n=0..23, with index 0 = off.
   Weber fraction k=0.162 → each step is ~1.2× JND at all brightness levels.
   Index 19→20 (120→140) = 20 PWM step at mid-range. */
static const uint8_t led_table[LED_TABLE_SIZE] PROGMEM =
{ 0,
  8,   9,  11,  13,  15,  17,  20,  23,  27,
  31,  36,  42,  49,  57,  66,  77,  89, 103,
  120, 140, 162, 189, 219, 255
};

static uint8_t led_index   = LED_INDEX_DEFAULT;
static uint8_t led_level_a;
static uint8_t led_level_b;
bool           led_enabled = true;

static void led_write(uint8_t a, uint8_t b)
{ TCA0.SPLIT.HCMP1 = a;  // PA4 / WO4 — takes effect at next HCNT underflow
  TCA0.SPLIT.HCMP2 = b;  // PA5 / WO5
}

void led_init(void)
{ LED_PORT.DIRSET = (1 << LED_A_PIN) | (1 << LED_B_PIN);

  // PORTA is the hardware default for TCA0 — stated explicitly for clarity
  PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTA_gc;

  // Enable split mode first — must precede all other TCA0 register writes
  TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

  TCA0.SPLIT.HPER = LED_BRIGHTNESS_MAX;  // 8-bit period for high counter

  led_level_a = pgm_read_byte(&led_table[led_index]);
  led_level_b = led_level_a;
  led_write(led_level_a, led_level_b);

  TCA0.SPLIT.CTRLB = TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm;

  // f_PWM = 24 MHz / (64 × 256) ≈ 1.465 kHz
  TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm;
}

void led_set(uint8_t index)
{ if(index >= LED_TABLE_SIZE) index = LED_TABLE_SIZE - 1;
  led_index   = index;
  led_level_a = pgm_read_byte(&led_table[led_index]);
  led_level_b = led_level_a;
  if(led_enabled) led_write(led_level_a, led_level_b);
}

void led_step(bool dir)
{ if(dir)
  { if(led_index < LED_TABLE_SIZE - 1) led_index++;
  }
  else
  { if(led_index > 0) led_index--;
  }
  led_level_a = pgm_read_byte(&led_table[led_index]);
  led_level_b = led_level_a;
  if(led_enabled) led_write(led_level_a, led_level_b);
}

void led_update_layer(uint8_t layer)
{ uint8_t base  = pgm_read_byte(&led_table[led_index]);
  uint8_t bi    = (led_index + LED_DELTA_STEPS < LED_TABLE_SIZE)
                  ? led_index + LED_DELTA_STEPS
                  : LED_TABLE_SIZE - 1;
  uint8_t boost = pgm_read_byte(&led_table[bi]);

  switch(layer)
  { case 0:
      led_level_a = base;
      led_level_b = base;
      break;
    case 1:
      led_level_a = boost;
      led_level_b = base;
      break;
    default:
      led_level_a = boost;
      led_level_b = boost;
      break;
  }
  if(led_enabled) led_write(led_level_a, led_level_b);
}

void led_off(void)
{ led_enabled = false;
  TCA0.SPLIT.CTRLB &= ~(TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm);
  LED_PORT.OUTCLR = (1 << LED_A_PIN) | (1 << LED_B_PIN);
}

void led_on(void)
{ led_enabled = true;
  led_write(led_level_a, led_level_b);
  TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP1EN_bm | TCA_SPLIT_HCMP2EN_bm;
}

/* Called once per 1 kHz tick after led_update_layer() has set led_level_a/b.
   Reads those layer-feedback values and overrides with full brightness for any
   active host lock LED, without disturbing the stored brightness state. */
void led_apply_host_report(uint8_t led_report)
{ uint8_t a = led_level_a;
  uint8_t b = led_level_b;
  if(led_report & 0x01u) a = LED_BRIGHTNESS_MAX;   /* Num Lock  → LED A full */
  if(led_report & 0x02u) b = LED_BRIGHTNESS_MAX;   /* Caps Lock → LED B full */
  if(led_enabled) led_write(a, b);
}
