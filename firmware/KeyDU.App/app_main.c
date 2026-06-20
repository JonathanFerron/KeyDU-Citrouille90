/* main.c — KeyDU.App keyboard firmware entry point
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   System init order: clock first (fixes MCLKTIMEBASE for USB and TCA0),
   then TCB0, then all keyboard subsystems.

   Main loop: IDLE sleep gated on a TCB0 1 ms tick flag. USB is fully
   interrupt-driven; no usb_task() polling is needed in the loop body.
   sleep_cpu() wakes on any interrupt (TCB0 overflow, USB SOF, bus events).
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>     /* wdt_disable() — guards against a previously armed WDT */
#include <stdint.h>

#include "clock.h"
#include "keyboard.h"
#include "usb_ctrl.h"

/* ── TCB0 1 ms tick flag ──────────────────────────────────────────────── */
static volatile uint8_t s_tick_flag;

ISR(TCB0_INT_vect)
{
  s_tick_flag++; // Increment instead of setting to 1, for debugging only
  TCB0.INTFLAGS = TCB_CAPT_bm;    /* clear interrupt flag */

} // ISR TCB0

ISR(BADISR_vect)
{ /* Unexpected interrupt — return silently.
     avr-libc default would reset via address 0 (lands in bootloader). */
}

/* ── main ─────────────────────────────────────────────────────────────── */
int main(void)
{ wdt_disable();

  /* clock_init() MUST be first: sets MCLKTIMEBASE=24, required by USB
   a nd TCA0 PWM.  TCB0 CCMP is also computed from *F_CPU so the clock
   must be stable before the timer is configured.  */
  clock_init();

  /* TCB0: periodic interrupt, 1 ms period, CLK_PER (24 MHz), no prescaler.
   CCMP = (F_CPU / 1000) - 1 = 23999.             *                    */
  TCB0.CCMP    = (F_CPU / 1000UL) - 1;
  TCB0.INTCTRL = TCB_CAPT_bm;
  TCB0.CTRLA   = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;

  keyboard_init();

  usb_init(USB_OPT_VREG_ENABLE);

  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_enable();

  sei(); // enable global interrupts

  // uint8_t i = 0;
  //PORTF.DIRSET = PIN2_bm;  // use PF2 (LED pin) for debugging on the CNano
  //PORTF.OUTSET = PIN2_bm;

  while(1)
  { usb_ctrl_poll();             // ungated — polls EP0 for SETUP packets


    if(s_tick_flag)
    {
      s_tick_flag = 0;

      // keyboard_task();  // commented out to make usb enumeration debug work easier: TODO: uncomment


    } // if(s_tick_flag)
    //sleep_cpu();    // IDLE: wakes on TCB0, USB SOF, USB bus events
  } // while

} // main
