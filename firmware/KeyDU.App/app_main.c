// main.c - KeyDU.App Keyboard Firmware
// Main entry point and system initialization

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "config.h"
#include "keyboard.h"

// Millisecond counter for timing
volatile uint32_t system_millis = 0;

// Timer interrupt for 1ms tick (using TCB0)
ISR(TCB0_INT_vect) {
    system_millis++;
    TCB0.INTFLAGS = TCB_CAPT_bm;  // Clear interrupt flag
}

// Public function to get current milliseconds
uint32_t millis(void) {
    uint32_t ms;
    cli();
    ms = system_millis;
    sei();
    return ms;
}

void system_init(void) {
    // Configure system clock (AVR64DU32 defaults to 24MHz internal oscillator)
    // No need to change if using default
    
    // Configure 1ms timer using TCB0
    TCB0.CCMP = (F_CPU / 1000) - 1;  // 1ms period
    TCB0.INTCTRL = TCB_CAPT_bm;      // Enable compare interrupt
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;  // Enable with no prescaler
    
    // Enable global interrupts
    sei();
}

int main(void) {
  // Disable watchdog timer: necessary?
  wdt_disable();
  
  // Initialize system
  system_init();
  
  // Initialize keyboard
  keyboard_init();  // split out usb_init()
  
  // Wait for USB to be ready
  _delay_ms(100);
  
  
  uint32_t last_tick = system_millis;

  // Main loop
  while (1) {
  uint32_t now = millis();
  if (now != last_tick) {
    last_tick = now;
    keyboard_task(); // split out usb_task()
    }
  // CPU idles here — add sleep_cpu() later for power saving
  }
    
  return 0;
}
