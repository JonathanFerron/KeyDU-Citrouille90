/* main.c — KeyDU.BL Bootloader Entry Point
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Boot decision — both conditions must hold to enter the bootloader:
 *
 *   1. Reset source was software reset (SWRF) or watchdog reset (WDRF).
 *      Power-on (PORF) and external RESET pin (EXTRF) always go to the app.
 *
 *   2. GPR.GPR0 == BOOT_MAGIC (0x42) AND GPR.GPR1 == ~BOOT_MAGIC (0xBD).
 *      Written by keyboard.c SYS_BOOT handler before triggering the WDT reset.
 *
 * RSTFR is read and stored immediately, then cleared by writing back the
 * observed bits.  Clearing is mandatory — flags accumulate across resets,
 * so a stale SWRF would otherwise survive into a later power-on reset.
 *
 * GPR is cleared before any jump so that subsequent resets start clean
 * regardless of which path is taken.
 *
 * Jump address: APP_START = 0x2000 (byte address) → 0x1000 (word address).
 * Cast via uintptr_t to suppress pointer-from-integer warnings.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "usbvendor/usb_vendor.h"   /* usb_vendor_init(), usb_vendor_task() */
#include "../../avrducore/ccp.h"     // ccp_write_ioreg()
#include "bootmagic.h"

/* ============================================================================
 * jump_to_application — clear GPR and transfer control to 0x2000
 * ========================================================================= */
static void jump_to_application(void) __attribute__((noreturn));
static void jump_to_application(void)
{
    /* Clear magic so a subsequent plain reset stays in the app. */
    GPR.GPR2 = 0x00u;
    GPR.GPR3 = 0x00u;

    /* Disable USB and interrupts before handing off. */
    cli();
    USB0.CTRLB  &= ~USB_ATTACH_bm;
    USB0.CTRLA  &= ~USB_ENABLE_bm;
    ccp_write_ioreg((void *)&CPUINT.CTRLA, 0x00);

    /* Jump to application reset vector (word address). */
    void (*app)(void) = (void (*)(void))(uintptr_t)((uint32_t)(APP_START) >> 1);
    app();

    /* Unreachable — silence noreturn warning. */
    while (1) {}
}

/* ============================================================================
 * main — boot decision then either BL loop or immediate app jump
 * ========================================================================= */
int main(void)
{
    /* Step 1: Read and immediately clear RSTFR.
     * Must be done before any peripheral init.  Writing back the observed
     * bits clears only the flags that were set — others are unaffected.
     * Clearing prevents stale flags from accumulating across reset cycles. */
    uint8_t rstfr = RSTCTRL.RSTFR;
    RSTCTRL.RSTFR = rstfr;

    /* Step 2: Read and clear GPR magic. */
    uint8_t gpr2 = GPR.GPR2;
    uint8_t gpr3 = GPR.GPR3;
    GPR.GPR2 = 0x00u;
    GPR.GPR3 = 0x00u;

    /* Step 3: Enter bootloader only if:
     *   - Reset was software-triggered (intentional programmatic reset)
     *   - GPR magic is intact (written by SYS_BOOT handler in keyboard.c)
     * Power-on reset (PORF) and RESET pin (EXTRF) always go straight to the app. */    
    bool soft = (rstfr & RSTCTRL_SWRF_bm ) != 0u;
    bool magic_valid = (gpr2 == BOOT_MAGIC) && (gpr3 == BOOT_MAGIC_COMPL);

    if (soft && magic_valid) {
      ccp_write_ioreg((void *)&CPUINT.CTRLA, CPUINT_IVSEL_bm);
      usb_vendor_init();   /* clock, USB hardware, state init, sei() */
      usb_vendor_task();   /* bare loop — never returns */
    }

    /* No valid boot request — jump straight to application. */
    jump_to_application();
}
