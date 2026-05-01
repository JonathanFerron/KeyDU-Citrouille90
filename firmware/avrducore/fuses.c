#include <avr/io.h>

/* AVR64DU32 fuse configuration.
 *
 * These are consumed by avr-objcopy at link time and written to the .fuses
 * section in the ELF. avrdude programs them into the device fuse
 * row separately from flash.
 *
 * BOOTSIZE / CODESIZE: 0x10 means 8KB boot partition —
 * 56 KB of flash is available to the application.
 *
 * USBSINK_ENABLE: required — enables the internal DM pull-down that
 * the AVR64DU32 USB PHY needs to operate as a full-speed device.
 *
 * RSTPINCFG_GPIO: Reset pin used as external reset.
 *
 * WDT: off. Enable in a production build if desired.
 * BOD: off. AVR64DU32 operates from 1.8–5.5 V; BOD adds quiescent draw.
 * 
 * The FUSES macro from avr-libc places the fuse data into a special ELF section called .fuse. It will be in your KeyDU.BL.elf already — you can verify:
 * avr-objdump -s -j .fuse KeyDU.BL.elf
 * 
 * You can verify it made it into the merged HEX:
 * srec_info KeyDU.merged.hex -Intel
 */

FUSES = {
    .BODCFG   = ACTIVE_DISABLE_gc | LVL_BODLEVEL0_gc |
                SAMPFREQ_128Hz_gc | SLEEP_DISABLE_gc,
    .BOOTSIZE  = 0x10,
    .CODESIZE  = 0x0,
    .OSCCFG   = CLKSEL_OSCHF_gc,
    .PDICFG   = KEY_NOTACT_gc | LEVEL_BASIC_gc,
    .SYSCFG0  = CRCSEL_CRC16_gc | CRCSRC_NOCRC_gc |
                RSTPINCFG_RST_gc | UPDIPINCFG_UPDI_gc,
    .SYSCFG1  = SUT_0MS_gc | USBSINK_ENABLE_gc,
    .WDTCFG   = PERIOD_OFF_gc | WINDOW_OFF_gc,
};
