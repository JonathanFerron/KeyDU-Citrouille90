/* main.c — KeyDU.BL Bootloader Entry Point
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   Boot decision — both conditions must hold to enter the bootloader:

     1. Reset source was software reset (SWRF)
        Power-on (PORF) and external RESET pin (EXTRF) always go to the app.

     2. EEPROM byte 0x00 == BOOT_MAGIC
        Written by keyboard.c SYS_BOOT handler before triggering the reset.

   RSTFR is read and stored immediately, then cleared by writing back the
   observed bits.  Clearing is mandatory — flags accumulate across resets,
   so a stale SWRF would otherwise survive into a later power-on reset.

   EEPROM is cleared before any jump so that subsequent resets start clean
   regardless of which path is taken.

   Jump address: APP_START = 0x2000 (byte address) → 0x1000 (word address).
   Cast via uintptr_t to suppress pointer-from-integer warnings.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/eeprom.h>

#include "usb_vendor.h"   /* usb_vendor_init(), usb_vendor_task() */
#include "../avrducore/ccp.h"     // ccp_write_ioreg()
#include "../avrducore/bootmagic.h"

/* ============================================================================
   jump_to_application — clear EEPROM and transfer control to 0x2000
   ========================================================================= */
static void jump_to_application(void) __attribute__((noreturn));
static void jump_to_application(void)
{ /* Clear magic so a subsequent plain reset stays in the app. */
  if(eeprom_read_byte(BOOT_MAGIC_EEPROM_ADDR) != 0xFFu)
    eeprom_write_byte(BOOT_MAGIC_EEPROM_ADDR, 0xFFu);

  /* Disable USB and interrupts before handing off. */
  cli();  // disable global interrupts
  USB0.CTRLB  &= ~USB_ATTACH_bm;
  USB0.CTRLA  &= ~USB_ENABLE_bm;
  ccp_write_ioreg((void*)&CPUINT.CTRLA, 0x00); // resets interupt vector select to default (appcode)

  /* Jump to application reset vector (word address). */
  void (*app)(void) = (void (*)(void))(uintptr_t)((uint32_t)(APP_START) >> 1);
  app();

  /* Unreachable — silence noreturn warning. */
  while(1) {}
}

/* ============================================================================
   main — boot decision then either BL loop or immediate app jump
   ========================================================================= */
int main(void)
{

  /* Blink PF2 (Nano LED0) — confirms app is reached and clock works : TODO, remove this once issue is resolved */
  PORTF.DIRSET = (1 << 2);
  for(uint8_t i = 0; i < 6; i++)
  { PORTF.OUTTGL = (1 << 2);
    for(volatile uint32_t d = 0; d < 200000UL; d++) {}
  }

  /* Step 1: Read and immediately clear RSTFR.
     Must be done before any peripheral init.  Writing back the observed
     bits clears only the flags that were set — others are unaffected.
     Clearing prevents stale flags from accumulating across reset cycles. */
  uint8_t rstfr = RSTCTRL.RSTFR;
  RSTCTRL.RSTFR = rstfr;

  /* Step 2: Read and clear EEPROM magic. */
  uint8_t magic = eeprom_read_byte(BOOT_MAGIC_EEPROM_ADDR);
  if(magic == BOOT_MAGIC)
    eeprom_write_byte(BOOT_MAGIC_EEPROM_ADDR, 0xFFu);

  /* Step 3: Enter bootloader only if:
       - Reset was software-triggered (intentional programmatic reset)
       - EEPROM magic is intact (written by SYS_BOOT handler in keyboard.c)
     Power-on reset (PORF) and RESET pin (EXTRF) always go straight to the app. */
  bool soft = (rstfr & RSTCTRL_SWRF_bm) != 0u;
  bool magic_valid = (magic == BOOT_MAGIC);

  if(soft && magic_valid)
  { ccp_write_ioreg((void*)&CPUINT.CTRLA, CPUINT_IVSEL_bm);  // sets interrupt vector to bootcode
    usb_vendor_init();   /* clock, USB hardware, state init, sei() */
    usb_vendor_task();   /* bare loop — never returns */
  }

  /* No valid boot request — jump straight to application. */
  jump_to_application();
}
